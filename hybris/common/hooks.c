/*
 * Copyright (c) 2012 Carsten Munk <carsten.munk@gmail.com>
 * Copyright (c) 2012 Canonical Ltd
 * Copyright (c) 2013 Christophe Chapuis <chris.chapuis@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include <hybris/internal/floating_point_abi.h>

#include "hooks_shm.h"

#define _GNU_SOURCE
#include <stdio.h>
#include <stdarg.h>
#include <stdio_ext.h>
#include <stddef.h>
#include <stdlib.h>
#include <limits.h>
#include <malloc.h>
#include <string.h>
#include <strings.h>
#include <dlfcn.h>
#include <pthread.h>
#include <signal.h>
#include <errno.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/xattr.h>
#include <grp.h>

#include <sys/ipc.h>
#include <sys/shm.h>

#include <linux/futex.h>
#include <sys/syscall.h>
#include <sys/time.h>

#include <netdb.h>
#include <unistd.h>
#include <syslog.h>
#include <locale.h>

#include <hybris/properties/properties.h>

static locale_t hybris_locale;
static int locale_inited = 0;
/* TODO:
*  - Check if the int arguments at attr_set/get match the ones at Android
*  - Check how to deal with memory leaks (specially with static initializers)
*  - Check for shared rwlock
*/

/* Base address to check for Android specifics */
#define ANDROID_TOP_ADDR_VALUE_MUTEX  0xFFFF
#define ANDROID_TOP_ADDR_VALUE_COND   0xFFFF
#define ANDROID_TOP_ADDR_VALUE_RWLOCK 0xFFFF

#define ANDROID_MUTEX_SHARED_MASK      0x2000
#define ANDROID_COND_SHARED_MASK       0x0001
#define ANDROID_COND_COUNTER_INCREMENT 0x0002
#define ANDROID_COND_COUNTER_MASK      (~ANDROID_COND_SHARED_MASK)
#define ANDROID_RWLOCKATTR_SHARED_MASK 0x0010

#define ANDROID_COND_IS_SHARED(c)  (((c)->value & ANDROID_COND_SHARED_MASK) != 0)

/* For the static initializer types */
#define ANDROID_PTHREAD_MUTEX_INITIALIZER            0
#define ANDROID_PTHREAD_RECURSIVE_MUTEX_INITIALIZER  0x4000
#define ANDROID_PTHREAD_ERRORCHECK_MUTEX_INITIALIZER 0x8000
#define ANDROID_PTHREAD_COND_INITIALIZER             0
#define ANDROID_PTHREAD_RWLOCK_INITIALIZER           0

/* Debug */
#include "logging.h"
#define LOGD(message, ...) HYBRIS_DEBUG_LOG(HOOKS, message, ##__VA_ARGS__)

/* we have a value p:
 *  - if p <= ANDROID_TOP_ADDR_VALUE_MUTEX then it is an android mutex, not one we processed
 *  - if p > VMALLOC_END, then the pointer is not a result of malloc ==> it is an shm offset
 */

struct _hook {
    const char *name;
    void *func;
};

/* pthread cond struct as done in Android */
typedef struct {
    int volatile value;
} android_cond_t;

/* Helpers */
static int hybris_check_android_shared_mutex(unsigned int mutex_addr)
{
    /* If not initialized or initialized by Android, it should contain a low
     * address, which is basically just the int values for Android's own
     * pthread_mutex_t */
    if ((mutex_addr <= ANDROID_TOP_ADDR_VALUE_MUTEX) &&
                    (mutex_addr & ANDROID_MUTEX_SHARED_MASK))
        return 1;

    return 0;
}

static int hybris_check_android_shared_cond(unsigned int cond_addr)
{
    /* If not initialized or initialized by Android, it should contain a low
     * address, which is basically just the int values for Android's own
     * pthread_cond_t */
    if ((cond_addr <= ANDROID_TOP_ADDR_VALUE_COND) &&
                    (cond_addr & ANDROID_COND_SHARED_MASK))
        return 1;

    /* In case android is setting up cond_addr with a negative value,
     * used for error control */
    if (cond_addr > HYBRIS_SHM_MASK_TOP)
        return 1;

    return 0;
}

/* Based on Android's Bionic pthread implementation.
 * This is just needed when we have a shared cond with Android */
static int __android_pthread_cond_pulse(android_cond_t *cond, int counter)
{
    long flags;
    int fret;

    if (cond == NULL)
        return EINVAL;

    flags = (cond->value & ~ANDROID_COND_COUNTER_MASK);
    for (;;) {
        long oldval = cond->value;
        long newval = 0;
        /* In our case all we need to do is make sure the negative value
         * is under our range, which is the last 0xF from SHM_MASK */
        if (oldval < -12)
            newval = ((oldval + ANDROID_COND_COUNTER_INCREMENT) &
                            ANDROID_COND_COUNTER_MASK) | flags;
        else
            newval = ((oldval - ANDROID_COND_COUNTER_INCREMENT) &
                            ANDROID_COND_COUNTER_MASK) | flags;
        if (__sync_bool_compare_and_swap(&cond->value, oldval, newval))
            break;
    }

    int pshared = cond->value & ANDROID_COND_SHARED_MASK;
    fret = syscall(SYS_futex , &cond->value,
                   pshared ? FUTEX_WAKE : FUTEX_WAKE_PRIVATE, counter,
                   NULL, NULL, NULL);
    LOGD("futex based pthread_cond_*, value %d, counter %d, ret %d",
                                            cond->value, counter, fret);
    return 0;
}

int android_pthread_cond_broadcast(android_cond_t *cond)
{
    return __android_pthread_cond_pulse(cond, INT_MAX);
}

int android_pthread_cond_signal(android_cond_t *cond)
{
    return __android_pthread_cond_pulse(cond, 1);
}

static void hybris_set_mutex_attr(unsigned int android_value, pthread_mutexattr_t *attr)
{
    /* Init already sets as PTHREAD_MUTEX_NORMAL */
    pthread_mutexattr_init(attr);

    if (android_value & ANDROID_PTHREAD_RECURSIVE_MUTEX_INITIALIZER) {
        pthread_mutexattr_settype(attr, PTHREAD_MUTEX_RECURSIVE);
    } else if (android_value & ANDROID_PTHREAD_ERRORCHECK_MUTEX_INITIALIZER) {
        pthread_mutexattr_settype(attr, PTHREAD_MUTEX_ERRORCHECK);
    }
}

static pthread_mutex_t* hybris_alloc_init_mutex(unsigned int android_mutex)
{
    pthread_mutex_t *realmutex = malloc(sizeof(pthread_mutex_t));
    pthread_mutexattr_t attr;
    hybris_set_mutex_attr(android_mutex, &attr);
    pthread_mutex_init(realmutex, &attr);
    return realmutex;
}

static pthread_cond_t* hybris_alloc_init_cond(void)
{
    pthread_cond_t *realcond = malloc(sizeof(pthread_cond_t));
    pthread_condattr_t attr;
    pthread_condattr_init(&attr);
    pthread_cond_init(realcond, &attr);
    return realcond;
}

static pthread_rwlock_t* hybris_alloc_init_rwlock(void)
{
    pthread_rwlock_t *realrwlock = malloc(sizeof(pthread_rwlock_t));
    pthread_rwlockattr_t attr;
    pthread_rwlockattr_init(&attr);
    pthread_rwlock_init(realrwlock, &attr);
    return realrwlock;
}

/*
 * utils, such as malloc, memcpy
 *
 * Useful to handle hacks such as the one applied for Nvidia, and to
 * avoid crashes.
 *
 * */

static void *my_malloc(size_t size)
{
    return malloc(size);
}

static void *my_memcpy(void *dst, const void *src, size_t len)
{
    if (src == NULL || dst == NULL)
        return NULL;

    return memcpy(dst, src, len);
}

static size_t my_strlen(const char *s)
{

    if (s == NULL)
        return -1;

    return strlen(s);
}

/*
 * Main pthread functions
 *
 * Custom implementations to workaround difference between Bionic and Glibc.
 * Our own pthread_create helps avoiding direct handling of TLS.
 *
 * */

static int my_pthread_create(pthread_t *thread, const pthread_attr_t *__attr,
                             void *(*start_routine)(void*), void *arg)
{
    pthread_attr_t *realattr = NULL;

    if (__attr != NULL)
        realattr = (pthread_attr_t *) *(unsigned int *) __attr;

    return pthread_create(thread, realattr, start_routine, arg);
}

/*
 * pthread_attr_* functions
 *
 * Specific implementations to workaround the differences between at the
 * pthread_attr_t struct differences between Bionic and Glibc.
 *
 * */

static int my_pthread_attr_init(pthread_attr_t *__attr)
{
    pthread_attr_t *realattr;

    realattr = malloc(sizeof(pthread_attr_t));
    *((unsigned int *)__attr) = (unsigned int) realattr;

    return pthread_attr_init(realattr);
}

static int my_pthread_attr_destroy(pthread_attr_t *__attr)
{
    int ret;
    pthread_attr_t *realattr = (pthread_attr_t *) *(unsigned int *) __attr;

    ret = pthread_attr_destroy(realattr);
    /* We need to release the memory allocated at my_pthread_attr_init
     * Possible side effects if destroy is called without our init */
    free(realattr);

    return ret;
}

static int my_pthread_attr_setdetachstate(pthread_attr_t *__attr, int state)
{
    pthread_attr_t *realattr = (pthread_attr_t *) *(unsigned int *) __attr;
    return pthread_attr_setdetachstate(realattr, state);
}

static int my_pthread_attr_getdetachstate(pthread_attr_t const *__attr, int *state)
{
    pthread_attr_t *realattr = (pthread_attr_t *) *(unsigned int *) __attr;
    return pthread_attr_getdetachstate(realattr, state);
}

static int my_pthread_attr_setschedpolicy(pthread_attr_t *__attr, int policy)
{
    pthread_attr_t *realattr = (pthread_attr_t *) *(unsigned int *) __attr;
    return pthread_attr_setschedpolicy(realattr, policy);
}

static int my_pthread_attr_getschedpolicy(pthread_attr_t const *__attr, int *policy)
{
    pthread_attr_t *realattr = (pthread_attr_t *) *(unsigned int *) __attr;
    return pthread_attr_getschedpolicy(realattr, policy);
}

static int my_pthread_attr_setschedparam(pthread_attr_t *__attr, struct sched_param const *param)
{
    pthread_attr_t *realattr = (pthread_attr_t *) *(unsigned int *) __attr;
    return pthread_attr_setschedparam(realattr, param);
}

static int my_pthread_attr_getschedparam(pthread_attr_t const *__attr, struct sched_param *param)
{
    pthread_attr_t *realattr = (pthread_attr_t *) *(unsigned int *) __attr;
    return pthread_attr_getschedparam(realattr, param);
}

static int my_pthread_attr_setstacksize(pthread_attr_t *__attr, size_t stack_size)
{
    pthread_attr_t *realattr = (pthread_attr_t *) *(unsigned int *) __attr;
    return pthread_attr_setstacksize(realattr, stack_size);
}

static int my_pthread_attr_getstacksize(pthread_attr_t const *__attr, size_t *stack_size)
{
    pthread_attr_t *realattr = (pthread_attr_t *) *(unsigned int *) __attr;
    return pthread_attr_getstacksize(realattr, stack_size);
}

static int my_pthread_attr_setstackaddr(pthread_attr_t *__attr, void *stack_addr)
{
    pthread_attr_t *realattr = (pthread_attr_t *) *(unsigned int *) __attr;
    return pthread_attr_setstackaddr(realattr, stack_addr);
}

static int my_pthread_attr_getstackaddr(pthread_attr_t const *__attr, void **stack_addr)
{
    pthread_attr_t *realattr = (pthread_attr_t *) *(unsigned int *) __attr;
    return pthread_attr_getstackaddr(realattr, stack_addr);
}

static int my_pthread_attr_setstack(pthread_attr_t *__attr, void *stack_base, size_t stack_size)
{
    pthread_attr_t *realattr = (pthread_attr_t *) *(unsigned int *) __attr;
    return pthread_attr_setstack(realattr, stack_base, stack_size);
}

static int my_pthread_attr_getstack(pthread_attr_t const *__attr, void **stack_base, size_t *stack_size)
{
    pthread_attr_t *realattr = (pthread_attr_t *) *(unsigned int *) __attr;
    return pthread_attr_getstack(realattr, stack_base, stack_size);
}

static int my_pthread_attr_setguardsize(pthread_attr_t *__attr, size_t guard_size)
{
    pthread_attr_t *realattr = (pthread_attr_t *) *(unsigned int *) __attr;
    return pthread_attr_setguardsize(realattr, guard_size);
}

static int my_pthread_attr_getguardsize(pthread_attr_t const *__attr, size_t *guard_size)
{
    pthread_attr_t *realattr = (pthread_attr_t *) *(unsigned int *) __attr;
    return pthread_attr_getguardsize(realattr, guard_size);
}

static int my_pthread_attr_setscope(pthread_attr_t *__attr, int scope)
{
    pthread_attr_t *realattr = (pthread_attr_t *) *(unsigned int *) __attr;
    return pthread_attr_setscope(realattr, scope);
}

static int my_pthread_attr_getscope(pthread_attr_t const *__attr)
{
    int scope;
    pthread_attr_t *realattr = (pthread_attr_t *) *(unsigned int *) __attr;

    /* Android doesn't have the scope attribute because it always
     * returns PTHREAD_SCOPE_SYSTEM */
    pthread_attr_getscope(realattr, &scope);

    return scope;
}

static int my_pthread_getattr_np(pthread_t thid, pthread_attr_t *__attr)
{
    pthread_attr_t *realattr;

    realattr = malloc(sizeof(pthread_attr_t));
    *((unsigned int *)__attr) = (unsigned int) realattr;

    return pthread_getattr_np(thid, realattr);
}

/*
 * pthread_mutex* functions
 *
 * Specific implementations to workaround the differences between at the
 * pthread_mutex_t struct differences between Bionic and Glibc.
 *
 * */

static int my_pthread_mutex_init(pthread_mutex_t *__mutex,
                          __const pthread_mutexattr_t *__mutexattr)
{
    pthread_mutex_t *realmutex = NULL;

    int pshared = 0;
    if (__mutexattr)
        pthread_mutexattr_getpshared(__mutexattr, &pshared);

    if (!pshared) {
        /* non shared, standard mutex: use malloc */
        realmutex = malloc(sizeof(pthread_mutex_t));

        *((unsigned int *)__mutex) = (unsigned int) realmutex;
    }
    else {
        /* process-shared mutex: use the shared memory segment */
        hybris_shm_pointer_t handle = hybris_shm_alloc(sizeof(pthread_mutex_t));

        *((hybris_shm_pointer_t *)__mutex) = handle;

        if (handle)
            realmutex = (pthread_mutex_t *)hybris_get_shmpointer(handle);
    }

    return pthread_mutex_init(realmutex, __mutexattr);
}

static int my_pthread_mutex_destroy(pthread_mutex_t *__mutex)
{
    int ret;

    if (!__mutex)
        return EINVAL;

    pthread_mutex_t *realmutex = (pthread_mutex_t *) *(unsigned int *) __mutex;

    if (!realmutex)
        return EINVAL;

    if (!hybris_is_pointer_in_shm((void*)realmutex)) {
        ret = pthread_mutex_destroy(realmutex);
        free(realmutex);
    }
    else {
        realmutex = (pthread_mutex_t *)hybris_get_shmpointer((hybris_shm_pointer_t)realmutex);
        ret = pthread_mutex_destroy(realmutex);
    }

    *((unsigned int *)__mutex) = 0;

    return ret;
}

static int my_pthread_mutex_lock(pthread_mutex_t *__mutex)
{
    if (!__mutex) {
        LOGD("Null mutex lock, not locking.");
        return 0;
    }

    unsigned int value = (*(unsigned int *) __mutex);
    if (hybris_check_android_shared_mutex(value)) {
        LOGD("Shared mutex with Android, not locking.");
        return 0;
    }

    pthread_mutex_t *realmutex = (pthread_mutex_t *) value;
    if (hybris_is_pointer_in_shm((void*)value))
        realmutex = (pthread_mutex_t *)hybris_get_shmpointer((hybris_shm_pointer_t)value);

    if (value <= ANDROID_TOP_ADDR_VALUE_MUTEX) {
        realmutex = hybris_alloc_init_mutex(value);
        *((unsigned int *)__mutex) = (unsigned int) realmutex;
    }

    return pthread_mutex_lock(realmutex);
}

static int my_pthread_mutex_trylock(pthread_mutex_t *__mutex)
{
    unsigned int value = (*(unsigned int *) __mutex);

    if (hybris_check_android_shared_mutex(value)) {
        LOGD("Shared mutex with Android, not try locking.");
        return 0;
    }

    pthread_mutex_t *realmutex = (pthread_mutex_t *) value;
    if (hybris_is_pointer_in_shm((void*)value))
        realmutex = (pthread_mutex_t *)hybris_get_shmpointer((hybris_shm_pointer_t)value);

    if (value <= ANDROID_TOP_ADDR_VALUE_MUTEX) {
        realmutex = hybris_alloc_init_mutex(value);
        *((unsigned int *)__mutex) = (unsigned int) realmutex;
    }

    return pthread_mutex_trylock(realmutex);
}

static int my_pthread_mutex_unlock(pthread_mutex_t *__mutex)
{
    if (!__mutex) {
        LOGD("Null mutex lock, not unlocking.");
        return 0;
    }

    unsigned int value = (*(unsigned int *) __mutex);
    if (hybris_check_android_shared_mutex(value)) {
        LOGD("Shared mutex with Android, not unlocking.");
        return 0;
    }

    if (value <= ANDROID_TOP_ADDR_VALUE_MUTEX) {
        LOGD("Trying to unlock a lock that's not locked/initialized"
               " by Hybris, not unlocking.");
        return 0;
    }

    pthread_mutex_t *realmutex = (pthread_mutex_t *) value;
    if (hybris_is_pointer_in_shm((void*)value))
        realmutex = (pthread_mutex_t *)hybris_get_shmpointer((hybris_shm_pointer_t)value);

    return pthread_mutex_unlock(realmutex);
}

static int my_pthread_mutex_lock_timeout_np(pthread_mutex_t *__mutex, unsigned __msecs)
{
    struct timespec tv;
    pthread_mutex_t *realmutex;
    unsigned int value = (*(unsigned int *) __mutex);

    if (hybris_check_android_shared_mutex(value)) {
        LOGD("Shared mutex with Android, not lock timeout np.");
        return 0;
    }

    realmutex = (pthread_mutex_t *) value;

    if (value <= ANDROID_TOP_ADDR_VALUE_MUTEX) {
        realmutex = hybris_alloc_init_mutex(value);
        *((int *)__mutex) = (int) realmutex;
    }

    /* TODO: Android uses CLOCK_MONOTONIC here but I am not sure which one to use */
    clock_gettime(CLOCK_REALTIME, &tv);
    tv.tv_sec += __msecs/1000;
    tv.tv_nsec += (__msecs % 1000) * 1000000;
    if (tv.tv_nsec >= 1000000000) {
      tv.tv_sec++;
      tv.tv_nsec -= 1000000000;
    }

    return pthread_mutex_timedlock(realmutex, &tv);
}

static int my_pthread_mutexattr_setpshared(pthread_mutexattr_t *__attr,
                                           int pshared)
{
    return pthread_mutexattr_setpshared(__attr, pshared);
}

/*
 * pthread_cond* functions
 *
 * Specific implementations to workaround the differences between at the
 * pthread_cond_t struct differences between Bionic and Glibc.
 *
 * */

static int my_pthread_cond_init(pthread_cond_t *cond,
                                const pthread_condattr_t *attr)
{
    pthread_cond_t *realcond = NULL;

    int pshared = 0;

    if (attr)
        pthread_condattr_getpshared(attr, &pshared);

    if (!pshared) {
        /* non shared, standard cond: use malloc */
        realcond = malloc(sizeof(pthread_cond_t));

        *((unsigned int *) cond) = (unsigned int) realcond;
    }
    else {
        /* process-shared condition: use the shared memory segment */
        hybris_shm_pointer_t handle = hybris_shm_alloc(sizeof(pthread_cond_t));

        *((unsigned int *)cond) = (unsigned int) handle;

        if (handle)
            realcond = (pthread_cond_t *)hybris_get_shmpointer(handle);
    }

    return pthread_cond_init(realcond, attr);
}

static int my_pthread_cond_destroy(pthread_cond_t *cond)
{
    int ret;
    pthread_cond_t *realcond = (pthread_cond_t *) *(unsigned int *) cond;

    if (!realcond) {
      return EINVAL;
    }

    if (!hybris_is_pointer_in_shm((void*)realcond)) {
        ret = pthread_cond_destroy(realcond);
        free(realcond);
    }
    else {
        realcond = (pthread_cond_t *)hybris_get_shmpointer((hybris_shm_pointer_t)realcond);
        ret = pthread_cond_destroy(realcond);
    }

    *((unsigned int *)cond) = 0;

    return ret;
}

static int my_pthread_cond_broadcast(pthread_cond_t *cond)
{
    unsigned int value = (*(unsigned int *) cond);
    if (hybris_check_android_shared_cond(value)) {
        LOGD("Shared condition with Android, broadcasting with futex.");
        return android_pthread_cond_broadcast((android_cond_t *) cond);
    }

    pthread_cond_t *realcond = (pthread_cond_t *) value;
    if (hybris_is_pointer_in_shm((void*)value))
        realcond = (pthread_cond_t *)hybris_get_shmpointer((hybris_shm_pointer_t)value);

    if (value <= ANDROID_TOP_ADDR_VALUE_COND) {
        realcond = hybris_alloc_init_cond();
        *((unsigned int *) cond) = (unsigned int) realcond;
    }

    return pthread_cond_broadcast(realcond);
}

static int my_pthread_cond_signal(pthread_cond_t *cond)
{
    unsigned int value = (*(unsigned int *) cond);

    if (hybris_check_android_shared_cond(value)) {
        LOGD("Shared condition with Android, broadcasting with futex.");
        return android_pthread_cond_signal((android_cond_t *) cond);
    }

    pthread_cond_t *realcond = (pthread_cond_t *) value;
    if (hybris_is_pointer_in_shm((void*)value))
        realcond = (pthread_cond_t *)hybris_get_shmpointer((hybris_shm_pointer_t)value);

    if (value <= ANDROID_TOP_ADDR_VALUE_COND) {
        realcond = hybris_alloc_init_cond();
        *((unsigned int *) cond) = (unsigned int) realcond;
    }

    return pthread_cond_signal(realcond);
}

static int my_pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex)
{
    /* Both cond and mutex can be statically initialized, check for both */
    unsigned int cvalue = (*(unsigned int *) cond);
    unsigned int mvalue = (*(unsigned int *) mutex);

    if (hybris_check_android_shared_cond(cvalue) ||
        hybris_check_android_shared_mutex(mvalue)) {
        LOGD("Shared condition/mutex with Android, not waiting.");
        return 0;
    }

    pthread_cond_t *realcond = (pthread_cond_t *) cvalue;
    if (hybris_is_pointer_in_shm((void*)cvalue))
        realcond = (pthread_cond_t *)hybris_get_shmpointer((hybris_shm_pointer_t)cvalue);

    if (cvalue <= ANDROID_TOP_ADDR_VALUE_COND) {
        realcond = hybris_alloc_init_cond();
        *((unsigned int *) cond) = (unsigned int) realcond;
    }

    pthread_mutex_t *realmutex = (pthread_mutex_t *) mvalue;
    if (hybris_is_pointer_in_shm((void*)mvalue))
        realmutex = (pthread_mutex_t *)hybris_get_shmpointer((hybris_shm_pointer_t)mvalue);

    if (mvalue <= ANDROID_TOP_ADDR_VALUE_MUTEX) {
        realmutex = hybris_alloc_init_mutex(mvalue);
        *((unsigned int *) mutex) = (unsigned int) realmutex;
    }

    return pthread_cond_wait(realcond, realmutex);
}

static int my_pthread_cond_timedwait(pthread_cond_t *cond,
                pthread_mutex_t *mutex, const struct timespec *abstime)
{
    /* Both cond and mutex can be statically initialized, check for both */
    unsigned int cvalue = (*(unsigned int *) cond);
    unsigned int mvalue = (*(unsigned int *) mutex);

    if (hybris_check_android_shared_cond(cvalue) ||
         hybris_check_android_shared_mutex(mvalue)) {
        LOGD("Shared condition/mutex with Android, not waiting.");
        return 0;
    }

    pthread_cond_t *realcond = (pthread_cond_t *) cvalue;
    if (hybris_is_pointer_in_shm((void*)cvalue))
        realcond = (pthread_cond_t *)hybris_get_shmpointer((hybris_shm_pointer_t)cvalue);

    if (cvalue <= ANDROID_TOP_ADDR_VALUE_COND) {
        realcond = hybris_alloc_init_cond();
        *((unsigned int *) cond) = (unsigned int) realcond;
    }

    pthread_mutex_t *realmutex = (pthread_mutex_t *) mvalue;
    if (hybris_is_pointer_in_shm((void*)mvalue))
        realmutex = (pthread_mutex_t *)hybris_get_shmpointer((hybris_shm_pointer_t)mvalue);

    if (mvalue <= ANDROID_TOP_ADDR_VALUE_MUTEX) {
        realmutex = hybris_alloc_init_mutex(mvalue);
        *((unsigned int *) mutex) = (unsigned int) realmutex;
    }

    return pthread_cond_timedwait(realcond, realmutex, abstime);
}

static int my_pthread_cond_timedwait_relative_np(pthread_cond_t *cond,
                pthread_mutex_t *mutex, const struct timespec *reltime)
{
    /* Both cond and mutex can be statically initialized, check for both */
    unsigned int cvalue = (*(unsigned int *) cond);
    unsigned int mvalue = (*(unsigned int *) mutex);

    if (hybris_check_android_shared_cond(cvalue) ||
         hybris_check_android_shared_mutex(mvalue)) {
        LOGD("Shared condition/mutex with Android, not waiting.");
        return 0;
    }

    pthread_cond_t *realcond = (pthread_cond_t *) cvalue;
    if( hybris_is_pointer_in_shm((void*)cvalue) )
        realcond = (pthread_cond_t *)hybris_get_shmpointer((hybris_shm_pointer_t)cvalue);

    if (cvalue <= ANDROID_TOP_ADDR_VALUE_COND) {
        realcond = hybris_alloc_init_cond();
        *((unsigned int *) cond) = (unsigned int) realcond;
    }

    pthread_mutex_t *realmutex = (pthread_mutex_t *) mvalue;
    if (hybris_is_pointer_in_shm((void*)mvalue))
        realmutex = (pthread_mutex_t *)hybris_get_shmpointer((hybris_shm_pointer_t)mvalue);

    if (mvalue <= ANDROID_TOP_ADDR_VALUE_MUTEX) {
        realmutex = hybris_alloc_init_mutex(mvalue);
        *((unsigned int *) mutex) = (unsigned int) realmutex;
    }

    /* TODO: Android uses CLOCK_MONOTONIC here but I am not sure which one to use */
    struct timespec tv;
    clock_gettime(CLOCK_REALTIME, &tv);
    tv.tv_sec += reltime->tv_sec;
    tv.tv_nsec += reltime->tv_nsec;
    if (tv.tv_nsec >= 1000000000) {
      tv.tv_sec++;
      tv.tv_nsec -= 1000000000;
    }
    return pthread_cond_timedwait(realcond, realmutex, &tv);
}

/*
 * pthread_rwlockattr_* functions
 *
 * Specific implementations to workaround the differences between at the
 * pthread_rwlockattr_t struct differences between Bionic and Glibc.
 *
 * */

static int my_pthread_rwlockattr_init(pthread_rwlockattr_t *__attr)
{
    pthread_rwlockattr_t *realattr;

    realattr = malloc(sizeof(pthread_rwlockattr_t));
    *((unsigned int *)__attr) = (unsigned int) realattr;

    return pthread_rwlockattr_init(realattr);
}

static int my_pthread_rwlockattr_destroy(pthread_rwlockattr_t *__attr)
{
    int ret;
    pthread_rwlockattr_t *realattr = (pthread_rwlockattr_t *) *(unsigned int *) __attr;

    ret = pthread_rwlockattr_destroy(realattr);
    free(realattr);

    return ret;
}

static int my_pthread_rwlockattr_setpshared(pthread_rwlockattr_t *__attr,
                                            int pshared)
{
    pthread_rwlockattr_t *realattr = (pthread_rwlockattr_t *) *(unsigned int *) __attr;
    return pthread_rwlockattr_setpshared(realattr, pshared);
}

static int my_pthread_rwlockattr_getpshared(pthread_rwlockattr_t *__attr,
                                            int *pshared)
{
    pthread_rwlockattr_t *realattr = (pthread_rwlockattr_t *) *(unsigned int *) __attr;
    return pthread_rwlockattr_getpshared(realattr, pshared);
}

/*
 * pthread_rwlock_* functions
 *
 * Specific implementations to workaround the differences between at the
 * pthread_rwlock_t struct differences between Bionic and Glibc.
 *
 * */

static int my_pthread_rwlock_init(pthread_rwlock_t *__rwlock,
                                  __const pthread_rwlockattr_t *__attr)
{
    pthread_rwlock_t *realrwlock = NULL;
    pthread_rwlockattr_t *realattr = NULL;
    int pshared = 0;

    if (__attr != NULL)
        realattr = (pthread_rwlockattr_t *) *(unsigned int *) __attr;

    if (realattr)
        pthread_rwlockattr_getpshared(realattr, &pshared);

    if (!pshared) {
        /* non shared, standard rwlock: use malloc */
        realrwlock = malloc(sizeof(pthread_rwlock_t));

        *((unsigned int *) __rwlock) = (unsigned int) realrwlock;
    }
    else {
        /* process-shared condition: use the shared memory segment */
        hybris_shm_pointer_t handle = hybris_shm_alloc(sizeof(pthread_rwlock_t));

        *((unsigned int *)__rwlock) = (unsigned int) handle;

        if (handle)
            realrwlock = (pthread_rwlock_t *)hybris_get_shmpointer(handle);
    }

    return pthread_rwlock_init(realrwlock, realattr);
}

static int my_pthread_rwlock_destroy(pthread_rwlock_t *__rwlock)
{
    int ret;
    pthread_rwlock_t *realrwlock = (pthread_rwlock_t *) *(unsigned int *) __rwlock;

    if (!hybris_is_pointer_in_shm((void*)realrwlock)) {
        ret = pthread_rwlock_destroy(realrwlock);
        free(realrwlock);
    }
    else {
        ret = pthread_rwlock_destroy(realrwlock);
        realrwlock = (pthread_rwlock_t *)hybris_get_shmpointer((hybris_shm_pointer_t)realrwlock);
    }

    return ret;
}

static pthread_rwlock_t* hybris_set_realrwlock(pthread_rwlock_t *rwlock)
{
    unsigned int value = (*(unsigned int *) rwlock);
    pthread_rwlock_t *realrwlock = (pthread_rwlock_t *) value;
    if (hybris_is_pointer_in_shm((void*)value))
        realrwlock = (pthread_rwlock_t *)hybris_get_shmpointer((hybris_shm_pointer_t)value);

    if (realrwlock <= ANDROID_TOP_ADDR_VALUE_RWLOCK) {
        realrwlock = hybris_alloc_init_rwlock();
        *((unsigned int *)rwlock) = (unsigned int) realrwlock;
    }
    return realrwlock;
}

static int my_pthread_rwlock_rdlock(pthread_rwlock_t *__rwlock)
{
    pthread_rwlock_t *realrwlock = hybris_set_realrwlock(__rwlock);
    return pthread_rwlock_rdlock(realrwlock);
}

static int my_pthread_rwlock_tryrdlock(pthread_rwlock_t *__rwlock)
{
    pthread_rwlock_t *realrwlock = hybris_set_realrwlock(__rwlock);
    return pthread_rwlock_tryrdlock(realrwlock);
}

static int my_pthread_rwlock_timedrdlock(pthread_rwlock_t *__rwlock,
                                         __const struct timespec *abs_timeout)
{
    pthread_rwlock_t *realrwlock = hybris_set_realrwlock(__rwlock);
    return pthread_rwlock_timedrdlock(realrwlock, abs_timeout);
}

static int my_pthread_rwlock_wrlock(pthread_rwlock_t *__rwlock)
{
    pthread_rwlock_t *realrwlock = hybris_set_realrwlock(__rwlock);
    return pthread_rwlock_wrlock(realrwlock);
}

static int my_pthread_rwlock_trywrlock(pthread_rwlock_t *__rwlock)
{
    pthread_rwlock_t *realrwlock = hybris_set_realrwlock(__rwlock);
    return pthread_rwlock_trywrlock(realrwlock);
}

static int my_pthread_rwlock_timedwrlock(pthread_rwlock_t *__rwlock,
                                         __const struct timespec *abs_timeout)
{
    pthread_rwlock_t *realrwlock = hybris_set_realrwlock(__rwlock);
    return pthread_rwlock_timedwrlock(realrwlock, abs_timeout);
}

static int my_pthread_rwlock_unlock(pthread_rwlock_t *__rwlock)
{
    unsigned int value = (*(unsigned int *) __rwlock);
    if (value <= ANDROID_TOP_ADDR_VALUE_RWLOCK) {
        LOGD("Trying to unlock a rwlock that's not locked/initialized"
               " by Hybris, not unlocking.");
        return 0;
    }
    pthread_rwlock_t *realrwlock = (pthread_rwlock_t *) value;
    if (hybris_is_pointer_in_shm((void*)value))
        realrwlock = (pthread_rwlock_t *)hybris_get_shmpointer((hybris_shm_pointer_t)value);

    return pthread_rwlock_unlock(realrwlock);
}


static int my_set_errno(int oi_errno)
{
    errno = oi_errno;
    return -1;
}

/*
 * __isthreaded is used in bionic's stdio.h to choose between a fast internal implementation
 * and a more classic stdio function call.
 * For example:
 * #define  __sfeof(p)  (((p)->_flags & __SEOF) != 0)
 * #define  feof(p)     (!__isthreaded ? __sfeof(p) : (feof)(p))
 *
 * We see here that if __isthreaded is false, then it will use directly the bionic's FILE structure
 * instead of calling one of the hooked methods.
 * Therefore we need to set __isthreaded to true, even if we are not in a multi-threaded context.
 */
static int __my_isthreaded = 1;

/*
 * redirection for bionic's __sF, which is defined as:
 *   FILE __sF[3];
 *   #define stdin  &__sF[0];
 *   #define stdout &__sF[1];
 *   #define stderr &__sF[2];
 *   So the goal here is to catch the call to file methods where the FILE* pointer
 *   is either stdin, stdout or stderr, and translate that pointer to a valid glibc
 *   pointer.
 *   Currently, only fputs is managed.
 */
#define BIONIC_SIZEOF_FILE 84
static char my_sF[3*BIONIC_SIZEOF_FILE] = {0};
static FILE *_get_actual_fp(FILE *fp)
{
    char *c_fp = (char*)fp;
    if (c_fp == &my_sF[0])
        return stdin;
    else if (c_fp == &my_sF[BIONIC_SIZEOF_FILE])
        return stdout;
    else if (c_fp == &my_sF[BIONIC_SIZEOF_FILE*2])
        return stderr;

    return fp;
}

static void my_clearerr(FILE *fp)
{
    clearerr(_get_actual_fp(fp));
}

static int my_fclose(FILE *fp)
{
    return fclose(_get_actual_fp(fp));
}

static int my_feof(FILE *fp)
{
    return feof(_get_actual_fp(fp));
}

static int my_ferror(FILE *fp)
{
    return ferror(_get_actual_fp(fp));
}

static int my_fflush(FILE *fp)
{
    return fflush(_get_actual_fp(fp));
}

static int my_fgetc(FILE *fp)
{
    return fgetc(_get_actual_fp(fp));
}

static int my_fgetpos(FILE *fp, fpos_t *pos)
{
    return fgetpos(_get_actual_fp(fp), pos);
}

static char* my_fgets(char *s, int n, FILE *fp)
{
    return fgets(s, n, _get_actual_fp(fp));
}

FP_ATTRIB static int my_fprintf(FILE *fp, const char *fmt, ...)
{
    int ret = 0;

    va_list args;
    va_start(args,fmt);
    ret = vfprintf(_get_actual_fp(fp), fmt, args);
    va_end(args);

    return ret;
}

static int my_fputc(int c, FILE *fp)
{
    return fputc(c, _get_actual_fp(fp));
}

static int my_fputs(const char *s, FILE *fp)
{
    return fputs(s, _get_actual_fp(fp));
}

static size_t my_fread(void *ptr, size_t size, size_t nmemb, FILE *fp)
{
    return fread(ptr, size, nmemb, _get_actual_fp(fp));
}

static FILE* my_freopen(const char *filename, const char *mode, FILE *fp)
{
    return freopen(filename, mode, _get_actual_fp(fp));
}

FP_ATTRIB static int my_fscanf(FILE *fp, const char *fmt, ...)
{
    int ret = 0;

    va_list args;
    va_start(args,fmt);
    ret = vfscanf(_get_actual_fp(fp), fmt, args);
    va_end(args);

    return ret;
}

static int my_fseek(FILE *fp, long offset, int whence)
{
    return fseek(_get_actual_fp(fp), offset, whence);
}

static int my_fseeko(FILE *fp, off_t offset, int whence)
{
    return fseeko(_get_actual_fp(fp), offset, whence);
}

static int my_fsetpos(FILE *fp, const fpos_t *pos)
{
    return fsetpos(_get_actual_fp(fp), pos);
}

static long my_ftell(FILE *fp)
{
    return ftell(_get_actual_fp(fp));
}

static off_t my_ftello(FILE *fp)
{
    return ftello(_get_actual_fp(fp));
}

static size_t my_fwrite(const void *ptr, size_t size, size_t nmemb, FILE *fp)
{
    return fwrite(ptr, size, nmemb, _get_actual_fp(fp));
}

static int my_getc(FILE *fp)
{
    return getc(_get_actual_fp(fp));
}

static ssize_t my_getdelim(char ** lineptr, size_t *n, int delimiter, FILE * fp)
{
    return getdelim(lineptr, n, delimiter, _get_actual_fp(fp));
}

static ssize_t my_getline(char **lineptr, size_t *n, FILE *fp)
{
    return getline(lineptr, n, _get_actual_fp(fp));
}


static int my_putc(int c, FILE *fp)
{
    return putc(c, _get_actual_fp(fp));
}

static void my_rewind(FILE *fp)
{
    rewind(_get_actual_fp(fp));
}

static void my_setbuf(FILE *fp, char *buf)
{
    setbuf(_get_actual_fp(fp), buf);
}

static int my_setvbuf(FILE *fp, char *buf, int mode, size_t size)
{
    return setvbuf(_get_actual_fp(fp), buf, mode, size);
}

static int my_ungetc(int c, FILE *fp)
{
    return ungetc(c, _get_actual_fp(fp));
}

static int my_vfprintf(FILE *fp, const char *fmt, va_list arg)
{
    return vfprintf(_get_actual_fp(fp), fmt, arg);
}


static int my_vfscanf(FILE *fp, const char *fmt, va_list arg)
{
    return vfscanf(_get_actual_fp(fp), fmt, arg);
}

static int my_fileno(FILE *fp)
{
    return fileno(_get_actual_fp(fp));
}


static int my_pclose(FILE *fp)
{
    return pclose(_get_actual_fp(fp));
}

static void my_flockfile(FILE *fp)
{
    return flockfile(_get_actual_fp(fp));
}

static int my_ftrylockfile(FILE *fp)
{
    return ftrylockfile(_get_actual_fp(fp));
}

static void my_funlockfile(FILE *fp)
{
    return funlockfile(_get_actual_fp(fp));
}


static int my_getc_unlocked(FILE *fp)
{
    return getc_unlocked(_get_actual_fp(fp));
}

static int my_putc_unlocked(int c, FILE *fp)
{
    return putc_unlocked(c, _get_actual_fp(fp));
}

/* exists only on the BSD platform
static char* my_fgetln(FILE *fp, size_t *len)
{
    return fgetln(_get_actual_fp(fp), len);
}
*/
static int my_fpurge(FILE *fp)
{
    __fpurge(_get_actual_fp(fp));

    return 0;
}

static int my_getw(FILE *fp)
{
    return getw(_get_actual_fp(fp));
}

static int my_putw(int w, FILE *fp)
{
    return putw(w, _get_actual_fp(fp));
}

static void my_setbuffer(FILE *fp, char *buf, int size)
{
    setbuffer(_get_actual_fp(fp), buf, size);
}

static int my_setlinebuf(FILE *fp)
{
    setlinebuf(_get_actual_fp(fp));

    return 0;
}

long my_sysconf(int name)
{
  /*
   * bionic has different values for the values below.
   * TODO: compare the values between glibc and bionic and complete the mapping
   */
  switch (name) {
  case 0x27:
    return sysconf(_SC_PAGESIZE);
  case 0x28:
    return sysconf(_SC_PAGE_SIZE);
  case 0x60:
    return sysconf(_SC_NPROCESSORS_CONF);
  case 0x61:
    return sysconf(_SC_NPROCESSORS_ONLN);
  default:
    break;
  }


  long rv = sysconf(name);

#ifdef DEBUG
  if (rv == -1) {
    printf("sysconf failed for %li\n", name);
    exit(-1);
  }
#endif

  return rv;
}

FP_ATTRIB static double my_strtod(const char *nptr, char **endptr)
{
	if (locale_inited == 0)
	{
		hybris_locale = newlocale(LC_ALL_MASK, "C", 0);
		locale_inited = 1;
	}
	return strtod_l(nptr, endptr, hybris_locale);
}

static int __my_system_property_read(const void *pi, char *name, char *value)
{
    return property_get(name, value, NULL);
}

static int __my_system_property_get(const char *name, char *value)
{
    return property_get(name, value, NULL);
}

static int __my_system_property_foreach(void (*propfn)(const void *pi, void *cookie), void *cookie)
{
    return 0;
}

static const void *__my_system_property_find(const char *name)
{
    return NULL;
}

static unsigned int __my_system_property_serial(const void *pi)
{
    return 0;
}

static int __my_system_property_wait(const void *pi)
{
    return 0;
}

static int __my_system_property_update(void *pi, const char *value, unsigned int len)
{
    return 0;
}

static int __my_system_property_add(const char *name, unsigned int namelen, const char *value, unsigned int valuelen)
{
    return 0;
}

static unsigned int __my_system_property_wait_any(unsigned int serial)
{
    return 0;
}

static const void *__my_system_property_find_nth(unsigned n)
{
    return NULL;
}

extern int __cxa_atexit(void (*)(void*), void*, void*);

static struct _hook hooks[] = {
    {"property_get", property_get },
    {"property_set", property_set },
    {"getenv", getenv },
    {"printf", printf },
    {"malloc", my_malloc },
    {"free", free },
    {"calloc", calloc },
    {"cfree", cfree },
    {"realloc", realloc },
    {"memalign", memalign },
    {"valloc", valloc },
    {"pvalloc", pvalloc },
    {"fread", fread },
    {"getxattr", getxattr},
    /* string.h */
    {"memccpy",memccpy},
    {"memchr",memchr},
    {"memrchr",memrchr},
    {"memcmp",memcmp},
    {"memcpy",my_memcpy},
    {"memmove",memmove},
    {"memset",memset},
    {"memmem",memmem},
    //  {"memswap",memswap},
    {"index",index},
    {"rindex",rindex},
    {"strchr",strchr},
    {"strrchr",strrchr},
    {"strlen",my_strlen},
    {"strcmp",strcmp},
    {"strcpy",strcpy},
    {"strcat",strcat},
    {"strcasecmp",strcasecmp},
    {"strncasecmp",strncasecmp},
    {"strdup",strdup},
    {"strstr",strstr},
    {"strtok",strtok},
    {"strtok_r",strtok_r},
    {"strerror",strerror},
    {"strerror_r",strerror_r},
    {"strnlen",strnlen},
    {"strncat",strncat},
    {"strndup",strndup},
    {"strncmp",strncmp},
    {"strncpy",strncpy},
    {"strtod", my_strtod},
    //{"strlcat",strlcat},
    //{"strlcpy",strlcpy},
    {"strcspn",strcspn},
    {"strpbrk",strpbrk},
    {"strsep",strsep},
    {"strspn",strspn},
    {"strsignal",strsignal},
    {"getgrnam", getgrnam},
    {"strcoll",strcoll},
    {"strxfrm",strxfrm},
    /* strings.h */
    {"bcmp",bcmp},
    {"bcopy",bcopy},
    {"bzero",bzero},
    {"ffs",ffs},
    {"index",index},
    {"rindex",rindex},
    {"strcasecmp",strcasecmp},
    {"strncasecmp",strncasecmp},
    /* dirent.h */
    {"opendir", opendir},
    {"closedir", closedir},
    /* pthread.h */
    {"pthread_atfork", pthread_atfork},
    {"pthread_create", my_pthread_create},
    {"pthread_kill", pthread_kill},
    {"pthread_exit", pthread_exit},
    {"pthread_join", pthread_join},
    {"pthread_detach", pthread_detach},
    {"pthread_self", pthread_self},
    {"pthread_equal", pthread_equal},
    {"pthread_getschedparam", pthread_getschedparam},
    {"pthread_setschedparam", pthread_setschedparam},
    {"pthread_mutex_init", my_pthread_mutex_init},
    {"pthread_mutex_destroy", my_pthread_mutex_destroy},
    {"pthread_mutex_lock", my_pthread_mutex_lock},
    {"pthread_mutex_unlock", my_pthread_mutex_unlock},
    {"pthread_mutex_trylock", my_pthread_mutex_trylock},
    {"pthread_mutex_lock_timeout_np", my_pthread_mutex_lock_timeout_np},
    {"pthread_mutexattr_init", pthread_mutexattr_init},
    {"pthread_mutexattr_destroy", pthread_mutexattr_destroy},
    {"pthread_mutexattr_getttype", pthread_mutexattr_gettype},
    {"pthread_mutexattr_settype", pthread_mutexattr_settype},
    {"pthread_mutexattr_getpshared", pthread_mutexattr_getpshared},
    {"pthread_mutexattr_setpshared", my_pthread_mutexattr_setpshared},
    {"pthread_condattr_init", pthread_condattr_init},
    {"pthread_condattr_getpshared", pthread_condattr_getpshared},
    {"pthread_condattr_setpshared", pthread_condattr_setpshared},
    {"pthread_condattr_destroy", pthread_condattr_destroy},
    {"pthread_cond_init", my_pthread_cond_init},
    {"pthread_cond_destroy", my_pthread_cond_destroy},
    {"pthread_cond_broadcast", my_pthread_cond_broadcast},
    {"pthread_cond_signal", my_pthread_cond_signal},
    {"pthread_cond_wait", my_pthread_cond_wait},
    {"pthread_cond_timedwait", my_pthread_cond_timedwait},
    {"pthread_cond_timedwait_monotonic", my_pthread_cond_timedwait},
    {"pthread_cond_timedwait_monotonic_np", my_pthread_cond_timedwait},
    {"pthread_cond_timedwait_relative_np", my_pthread_cond_timedwait_relative_np},
    {"pthread_key_delete", pthread_key_delete},
    {"pthread_setname_np", pthread_setname_np},
    {"pthread_once", pthread_once},
    {"pthread_key_create", pthread_key_create},
    {"pthread_setspecific", pthread_setspecific},
    {"pthread_getspecific", pthread_getspecific},
    {"pthread_attr_init", my_pthread_attr_init},
    {"pthread_attr_destroy", my_pthread_attr_destroy},
    {"pthread_attr_setdetachstate", my_pthread_attr_setdetachstate},
    {"pthread_attr_getdetachstate", my_pthread_attr_getdetachstate},
    {"pthread_attr_setschedpolicy", my_pthread_attr_setschedpolicy},
    {"pthread_attr_getschedpolicy", my_pthread_attr_getschedpolicy},
    {"pthread_attr_setschedparam", my_pthread_attr_setschedparam},
    {"pthread_attr_getschedparam", my_pthread_attr_getschedparam},
    {"pthread_attr_setstacksize", my_pthread_attr_setstacksize},
    {"pthread_attr_getstacksize", my_pthread_attr_getstacksize},
    {"pthread_attr_setstackaddr", my_pthread_attr_setstackaddr},
    {"pthread_attr_getstackaddr", my_pthread_attr_getstackaddr},
    {"pthread_attr_setstack", my_pthread_attr_setstack},
    {"pthread_attr_getstack", my_pthread_attr_getstack},
    {"pthread_attr_setguardsize", my_pthread_attr_setguardsize},
    {"pthread_attr_getguardsize", my_pthread_attr_getguardsize},
    {"pthread_attr_setscope", my_pthread_attr_setscope},
    {"pthread_attr_setscope", my_pthread_attr_getscope},
    {"pthread_getattr_np", my_pthread_getattr_np},
    {"pthread_rwlockattr_init", my_pthread_rwlockattr_init},
    {"pthread_rwlockattr_destroy", my_pthread_rwlockattr_destroy},
    {"pthread_rwlockattr_setpshared", my_pthread_rwlockattr_setpshared},
    {"pthread_rwlockattr_getpshared", my_pthread_rwlockattr_getpshared},
    {"pthread_rwlock_init", my_pthread_rwlock_init},
    {"pthread_rwlock_destroy", my_pthread_rwlock_destroy},
    {"pthread_rwlock_unlock", my_pthread_rwlock_unlock},
    {"pthread_rwlock_wrlock", my_pthread_rwlock_wrlock},
    {"pthread_rwlock_rdlock", my_pthread_rwlock_rdlock},
    {"pthread_rwlock_tryrdlock", my_pthread_rwlock_tryrdlock},
    {"pthread_rwlock_trywrlock", my_pthread_rwlock_trywrlock},
    {"pthread_rwlock_timedrdlock", my_pthread_rwlock_timedrdlock},
    {"pthread_rwlock_timedwrlock", my_pthread_rwlock_timedwrlock},
    /* stdio.h */
    {"__isthreaded", &__my_isthreaded},
    {"__sF", &my_sF},
    {"fopen", fopen},
    {"fdopen", fdopen},
    {"popen", popen},
    {"puts", puts},
    {"sprintf", sprintf},
    {"asprintf", asprintf},
    {"vasprintf", vasprintf},
    {"snprintf", snprintf},
    {"vsprintf", vsprintf},
    {"vsnprintf", vsnprintf},
    {"clearerr", my_clearerr},
    {"fclose", my_fclose},
    {"feof", my_feof},
    {"ferror", my_ferror},
    {"fflush", my_fflush},
    {"fgetc", my_fgetc},
    {"fgetpos", my_fgetpos},
    {"fgets", my_fgets},
    {"fprintf", my_fprintf},
    {"fputc", my_fputc},
    {"fputs", my_fputs},
    {"fread", my_fread},
    {"freopen", my_freopen},
    {"fscanf", my_fscanf},
    {"fseek", my_fseek},
    {"fseeko", my_fseeko},
    {"fsetpos", my_fsetpos},
    {"ftell", my_ftell},
    {"ftello", my_ftello},
    {"fwrite", my_fwrite},
    {"getc", my_getc},
    {"getdelim", my_getdelim},
    {"getline", my_getline},
    {"putc", my_putc},
    {"rewind", my_rewind},
    {"setbuf", my_setbuf},
    {"setvbuf", my_setvbuf},
    {"ungetc", my_ungetc},
    {"vasprintf", vasprintf},
    {"vfprintf", my_vfprintf},
    {"vfscanf", my_vfscanf},
    {"fileno", my_fileno},
    {"pclose", my_pclose},
    {"flockfile", my_flockfile},
    {"ftrylockfile", my_ftrylockfile},
    {"funlockfile", my_funlockfile},
    {"getc_unlocked", my_getc_unlocked},
    {"putc_unlocked", my_putc_unlocked},
    //{"fgetln", my_fgetln},
    {"fpurge", my_fpurge},
    {"getw", my_getw},
    {"putw", my_putw},
    {"setbuffer", my_setbuffer},
    {"setlinebuf", my_setlinebuf},
    {"__errno", __errno_location},
    {"__set_errno", my_set_errno},
    /* net specifics, to avoid __res_get_state */
    {"getaddrinfo", getaddrinfo},
    {"gethostbyaddr", gethostbyaddr},
    {"gethostbyname", gethostbyname},
    {"gethostbyname2", gethostbyname2},
    {"gethostent", gethostent},
    {"strftime", strftime},
    {"sysconf", my_sysconf},
    {"sscanf", sscanf},
    {"scanf", scanf},
    {"vscanf", vscanf},
    {"vsscanf", vsscanf},
    {"openlog", openlog},
    {"syslog", syslog},
    {"closelog", closelog},
    {"vsyslog", vsyslog},
    {"timer_create", timer_create},
    {"timer_settime", timer_settime},
    {"timer_gettime", timer_gettime},
    {"timer_delete", timer_delete},
    {"timer_getoverrun", timer_getoverrun},
    {"abort", abort},
    {"writev", writev},
    /* unistd.h */
    {"access", access},
    /* grp.h */
    {"getgrgid", getgrgid},
    {"__cxa_atexit", __cxa_atexit},
    {"__system_property_read", __my_system_property_read},
    {"__system_property_get", __my_system_property_get},
    {"__system_property_set", property_set},
    {"__system_property_foreach", __my_system_property_foreach},
    {"__system_property_find", __my_system_property_find},
    {"__system_property_serial", __my_system_property_serial},
    {"__system_property_wait", __my_system_property_wait},
    {"__system_property_update", __my_system_property_update},
    {"__system_property_add", __my_system_property_add},
    {"__system_property_wait_any", __my_system_property_wait_any},
    {"__system_property_find_nth", __my_system_property_find_nth},
    {NULL, NULL},
};

void *get_hooked_symbol(char *sym)
{
    struct _hook *ptr = &hooks[0];
    static int counter = -1;

    while (ptr->name != NULL)
    {
        if (strcmp(sym, ptr->name) == 0){
            return ptr->func;
        }
        ptr++;
    }
    if (strstr(sym, "pthread") != NULL)
    {
        /* safe */
        if (strcmp(sym, "pthread_sigmask") == 0)
           return NULL;
        /* not safe */
        counter--;
        LOGD("%s %i\n", sym, counter);
        return (void *) counter;
    }
    return NULL;
}

void android_linker_init()
{
}
