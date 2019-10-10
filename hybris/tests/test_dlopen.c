#include <hybris/common/binding.h>
#include <dlfcn.h>
#include <EGL/egl.h>
#include <stdio.h>
#include <stddef.h>


int main(int argc, char **argv) {
    
    int i = 0;
    char * libname = "libc.so";

    if(argc > 1) {
        libname = argv[1];
    }

    void * handler = android_dlopen(libname, RTLD_LAZY);
    printf("android %s is %p\n", libname,handler);
    return 0;
}