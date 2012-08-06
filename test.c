#include <dlfcn.h>
#include <stddef.h>
int main(int argc, char **argv)
{
    void *foo = android_dlopen(argv[1], RTLD_LAZY);
    void* (*eglgetdisplay)(void *win);
    void* (*eglinitialize)(void *win, void *foo, void *bar);

    eglgetdisplay = android_dlsym(foo, "eglGetDisplay");
    eglinitialize = android_dlsym(foo, "eglInitialize");
    android_dlclose(foo);
    printf("full stop\n");    
exit(0); 
}  
