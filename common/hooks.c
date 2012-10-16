
#include "properties.h"
#define _GNU_SOURCE
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <strings.h>
#include <dlfcn.h>
#include <pthread.h>
#include <errno.h>

struct _hook {
  const char *name;
  void *func;
};

static int my_pthread_mutex_init (pthread_mutex_t *__mutex, __const pthread_mutexattr_t *__mutexattr)
{
  pthread_mutex_t *realmutex = malloc(sizeof(pthread_mutex_t));
  *((int *)__mutex) = (int) realmutex;
  printf("init %x\n", __mutex);
  return pthread_mutex_init(realmutex, __mutexattr);
}

static int my_pthread_mutex_lock (pthread_mutex_t *__mutex)
{
  pthread_mutex_t *realmutex = (pthread_mutex_t *) *(int *) __mutex;
  if (realmutex == NULL)
  {
      realmutex = malloc(sizeof(pthread_mutex_t));
      *((int *)__mutex) = (int) realmutex;
      pthread_mutex_init(realmutex, NULL);     
  }
  return pthread_mutex_lock(realmutex);
}

static int my_pthread_mutex_trylock (pthread_mutex_t *__mutex)
{
  pthread_mutex_t *realmutex = (pthread_mutex_t *) *(int *) __mutex;
  if (realmutex == NULL)
  {
      realmutex = malloc(sizeof(pthread_mutex_t));
      *((int *)__mutex) = (int) realmutex;
      pthread_mutex_init(realmutex, NULL);     
  }
  return pthread_mutex_trylock(realmutex);
}


static int my_pthread_mutex_unlock (pthread_mutex_t *__mutex)
{
  pthread_mutex_t *realmutex = (pthread_mutex_t *) *(int *) __mutex;
  return pthread_mutex_unlock(realmutex);
}

static int my_pthread_mutex_destroy (pthread_mutex_t *__mutex)
{
  pthread_mutex_t *realmutex = (pthread_mutex_t *) *(int *) __mutex;
  int ret = 0;
  ret = pthread_mutex_destroy(realmutex);
  free(realmutex);
  return ret;
}                               

static int my_pthread_cond_init(pthread_cond_t *cond, const pthread_condattr_t *attr)
{
  pthread_cond_t *realcond = malloc(sizeof(pthread_cond_t));
  *((int *) cond) = (int) realcond;
  return pthread_cond_init(realcond, attr);    
}

static int my_pthread_cond_destroy (pthread_cond_t *cond)
{
  pthread_cond_t *realcond = (pthread_cond_t *) *(int *) cond;
  int ret = 0;
  ret = pthread_cond_destroy(realcond);
  free(realcond);
  return ret;
}                               


static int my_pthread_cond_broadcast(pthread_cond_t *cond)
{
  pthread_cond_t *realcond = (pthread_cond_t *) *(int *) cond;
  return pthread_cond_broadcast(realcond);    
}

static int my_pthread_cond_signal(pthread_cond_t *cond)
{
  pthread_cond_t *realcond = (pthread_cond_t *) *(int *) cond;
  return pthread_cond_signal(realcond);    
}

static int my_pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex)
{
  pthread_cond_t *realcond = (pthread_cond_t *) *(int *) cond;

  pthread_mutex_t *realmutex = (pthread_mutex_t *) *(int *) mutex;
  return pthread_cond_wait(realcond, realmutex);    
}

static int my_pthread_cond_timedwait(pthread_cond_t *cond, pthread_mutex_t *mutex, const struct timespec *abstime)
{
  pthread_cond_t *realcond = (pthread_cond_t *) *(int *) cond;

  pthread_mutex_t *realmutex = (pthread_mutex_t *) *(int *) mutex;
  
  return pthread_cond_timedwait(realcond, realmutex, abstime);    
}

static int my_pthread_attr_destroy(pthread_attr_t *__attr)
{
  pthread_attr_t *realattr = (pthread_attr_t *) *(int *) __attr;
  return pthread_attr_destroy(realattr);
}

static int my_pthread_attr_setdetachstate(pthread_attr_t *__attr, int detachstate)
{
  pthread_attr_t *realattr = (pthread_attr_t *) *(int *) __attr;
  return pthread_attr_setdetachstate(realattr, detachstate);
}


static int my_pthread_attr_setstacksize(pthread_attr_t *__attr, size_t stacksize)
{

  pthread_attr_t *realattr = (pthread_attr_t *) *(int *) __attr;
  return pthread_attr_setstacksize (realattr,stacksize);
}

static int my_pthread_attr_init(pthread_attr_t *__attr) 
{
  pthread_attr_t *realattr = (pthread_attr_t *) *(int *) __attr;

  realattr = malloc(sizeof(pthread_attr_t));
  *((int *)__attr) = (int) realattr;

  return pthread_attr_init(realattr);
}

static int my_set_errno(int oi_errno)
{
    errno = oi_errno;
    return -1;
}

static struct _hook hooks[] = {
  {"property_get", property_get },
  {"property_set", property_set },
  {"printf", printf },
  {"malloc", malloc },
  {"free", free },
  {"calloc", calloc },
  {"cfree", cfree },
  {"realloc", realloc },
  {"memalign", memalign },
  {"valloc", valloc },
  {"pvalloc", pvalloc },
/* string.h */
  {"memccpy",memccpy}, 
  {"memchr",memchr}, 
  {"memrchr",memrchr}, 
  {"memcmp",memcmp}, 
  {"memcpy",memcpy}, 
  {"memmove",memmove}, 
  {"memset",memset}, 
  {"memmem",memmem}, 
//  {"memswap",memswap}, 
  {"index",index}, 
  {"rindex",rindex}, 
  {"strchr",strchr}, 
  {"strrchr",strrchr}, 
  {"strlen",strlen}, 
  {"strcmp",strcmp}, 
  {"strcpy",strcpy}, 
  {"strcat",strcat}, 
  {"strcasecmp",strcasecmp}, 
  {"strncasecmp",strncasecmp}, 
  {"strdup",strdup}, 
  {"strstr",strstr}, 
  {"strcasestr",strcasestr}, 
  {"strtok",strtok}, 
  {"strtok_r",strtok_r}, 
  {"strerror",strerror}, 
  {"strerror_r",strerror_r}, 
  {"strnlen",strnlen}, 
  {"strncat",strncat}, 
  {"strndup",strndup}, 
  {"strncmp",strncmp}, 
  {"strncpy",strncpy}, 
//  {"strlcat",strlcat}, 
//  {"strlcpy",strlcpy}, 
  {"strcspn",strcspn}, 
  {"strpbrk",strpbrk}, 
  {"strsep",strsep}, 
  {"strspn",strspn}, 
  {"strsignal",strsignal}, 
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
/* pthread.h */
  {"pthread_create", pthread_create},
  {"pthread_exit", pthread_exit},
  {"pthread_join", pthread_join},
  {"pthread_detach", pthread_detach},
  {"pthread_self", pthread_self},
  {"pthread_equal", pthread_equal},
  {"pthread_mutex_init", my_pthread_mutex_init },
  {"pthread_mutex_lock", my_pthread_mutex_lock },
  {"pthread_mutex_unlock", my_pthread_mutex_unlock },
  {"pthread_mutex_destroy", my_pthread_mutex_destroy },
  {"pthread_once", pthread_once},
  {"pthread_mutexattr_init", pthread_mutexattr_init},
  {"pthread_mutexattr_settype", pthread_mutexattr_settype},
  {"pthread_mutexattr_destroy", pthread_mutexattr_destroy},
  {"pthread_mutex_trylock", my_pthread_mutex_trylock},   
  {"pthread_key_create", pthread_key_create},
  {"pthread_key_delete", pthread_key_delete},
  {"pthread_setschedparam", pthread_setschedparam},
  {"pthread_getschedparam", pthread_getschedparam},
  {"pthread_setspecific", pthread_setspecific},
  {"pthread_getspecific", pthread_getspecific},
  {"pthread_cond_init", my_pthread_cond_init},
  {"pthread_cond_broadcast", my_pthread_cond_broadcast},
  {"pthread_cond_destroy", my_pthread_cond_destroy},
  {"pthread_cond_signal", my_pthread_cond_signal},
  {"pthread_cond_wait", my_pthread_cond_wait},
  {"pthread_cond_timedwait", my_pthread_cond_timedwait},
  {"pthread_attr_setstacksize", my_pthread_attr_setstacksize},
  {"pthread_attr_destroy", my_pthread_attr_destroy},
  {"pthread_attr_setdetachstate", my_pthread_attr_setdetachstate},
  {"pthread_attr_init", my_pthread_attr_init},
  {"pthread_rwlock_init", pthread_rwlock_init},
  {"pthread_rwlock_destroy", pthread_rwlock_destroy},
  {"pthread_rwlock_unlock", pthread_rwlock_unlock},
  {"pthread_rwlock_wrlock", pthread_rwlock_wrlock},
  {"pthread_rwlock_rdlock", pthread_rwlock_rdlock},
  {"__get_tls", -1},
  {"fopen", fopen},
  {"fgets", fgets},
  {"fclose", fclose},
  {"__errno", __errno_location},
  {"__set_errno", my_set_errno},
    {NULL, NULL},
};

void *get_hooked_symbol(char *sym)
{
  struct _hook *ptr = &hooks[0];
  static int counter = -1;  
  
  while (ptr->name != NULL)
  {
      if (strcmp(sym, ptr->name) == 0)
        return ptr->func;
      ptr++;
  }
  if (strstr(sym, "pthread") != NULL)
  {
     counter--;
     printf("%s %i\n", sym, counter);
     return (void *) counter;
  }
  return NULL;
}

void android_linker_init()
{
}

