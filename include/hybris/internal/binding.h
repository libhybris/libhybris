/*
 * Copyright (C) 2013 Simon Busch <morphis@gravedo.de>
 *               2012 Canonical Ltd
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef HYBRIS_BINDING_H_
#define HYBRIS_BINDING_H_

void *android_dlopen(const char *filename, int flag);
void *android_dlsym(void *name, const char *symbol);

#define HYBRIS_DLSYSM(name, fptr, sym) \
    if (!name##_handle) \
        hybris_##name##_initialize(); \
    if (*(fptr) == NULL) \
    { \
        *(fptr) = (void *) android_dlsym(name##_handle, sym); \
    }

#define HYBRIS_LIBRARY_INITIALIZE(name, path) \
    void *name##_handle; \
    void hybris_##name##_initialize() \
    { \
        name##_handle = android_dlopen(path, RTLD_LAZY); \
    }

#define HYBRIS_IMPLEMENT_FUNCTION0(name, return_type, symbol)  \
    return_type symbol()                          \
    {                                             \
        static return_type (*f)() = NULL;         \
        HYBRIS_DLSYSM(name, &f, #symbol);                \
        return f(); \
    }

#define HYBRIS_IMPLEMENT_FUNCTION1(name, return_type, symbol, arg1) \
    return_type symbol(arg1 _1)                        \
    {                                                  \
        static return_type (*f)(arg1) = NULL;          \
        HYBRIS_DLSYSM(name, &f, #symbol);                     \
        return f(_1); \
    }

#define HYBRIS_IMPLEMENT_FUNCTION2(name, return_type, symbol, arg1, arg2) \
    return_type symbol(arg1 _1, arg2 _2)                        \
    {                                                  \
        static return_type (*f)(arg1, arg2) = NULL;          \
        HYBRIS_DLSYSM(name, &f, #symbol);                     \
        return f(_1, _2); \
    }

#define HYBRIS_IMPLEMENT_FUNCTION3(name, return_type, symbol, arg1, arg2, arg3) \
    return_type symbol(arg1 _1, arg2 _2, arg3 _3)                        \
    {                                                  \
        static return_type (*f)(arg1, arg2, arg3) = NULL;          \
        HYBRIS_DLSYSM(name, &f, #symbol);                     \
        return f(_1, _2, _3); \
    }

#define HYBRIS_IMPLEMENT_FUNCTION4(name, return_type, symbol, arg1, arg2, arg3, arg4) \
    return_type symbol(arg1 _1, arg2 _2, arg3 _3, arg4 _4)                        \
    {                                                  \
        static return_type (*f)(arg1, arg2, arg3, arg4) = NULL;          \
        HYBRIS_DLSYSM(name, &f, #symbol);                     \
        return f(_1, _2, _3, _4); \
    }

#define HYBRIS_IMPLEMENT_FUNCTION5(name, return_type, symbol, arg1, arg2, arg3, arg4, arg5) \
    return_type symbol(arg1 _1, arg2 _2, arg3 _3, arg4 _4, arg5 _5)                        \
    {                                                  \
        static return_type (*f)(arg1, arg2, arg3, arg4, arg5) = NULL;          \
        HYBRIS_DLSYSM(name, &f, #symbol);                     \
        return f(_1, _2, _3, _4, _5); \
    }

#define HYBRIS_IMPLEMENT_FUNCTION6(name, return_type, symbol, arg1, arg2, arg3, arg4, arg5, arg6) \
    return_type symbol(arg1 _1, arg2 _2, arg3 _3, arg4 _4, arg5 _5, arg6 _6)                        \
    {                                                  \
        static return_type (*f)(arg1, arg2, arg3, arg4, arg5, arg6) = NULL;          \
        HYBRIS_DLSYSM(name, &f, #symbol);                     \
        return f(_1, _2, _3, _4, _5, _6); \
    }

#define HYBRIS_IMPLEMENT_FUNCTION7(name, return_type, symbol, arg1, arg2, arg3, arg4, arg5, arg6, arg7) \
    return_type symbol(arg1 _1, arg2 _2, arg3 _3, arg4 _4, arg5 _5, arg6 _6, arg7 _7)                        \
    {                                                  \
        static return_type (*f)(arg1, arg2, arg3, arg4, arg5, arg6, arg7) = NULL;          \
        HYBRIS_DLSYSM(name, &f, #symbol);                     \
        return f(_1, _2, _3, _4, _5, _6, _7); \
    }

#define HYBRIS_IMPLEMENT_VOID_FUNCTION1(name, symbol, arg1)               \
    void symbol(arg1 _1)                                     \
    {                                                        \
        static void (*f)(arg1) = NULL;                       \
        HYBRIS_DLSYSM(name, &f, #symbol);                           \
        f(_1); \
    }

#define HYBRIS_IMPLEMENT_VOID_FUNCTION2(name, symbol, arg1, arg2)            \
    void symbol(arg1 _1, arg2 _2)                               \
    {                                                           \
        static void (*f)(arg1, arg2) = NULL;                    \
        HYBRIS_DLSYSM(name, &f, #symbol);                              \
        f(_1, _2); \
    }

#define HYBRIS_IMPLEMENT_VOID_FUNCTION3(name, symbol, arg1, arg2, arg3)      \
    void symbol(arg1 _1, arg2 _2, arg3 _3)                      \
    {                                                           \
        static void (*f)(arg1, arg2, arg3) = NULL;              \
        HYBRIS_DLSYSM(name, &f, #symbol);                              \
        f(_1, _2, _3); \
    }


#endif
