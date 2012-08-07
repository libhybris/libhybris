#include <dlfcn.h>
#include <stddef.h>
#include <assert.h>
int main(int argc, char **argv)
{
    void *foo = android_dlopen(argv[1], RTLD_LAZY);
    void* (*hello)(char a, char b);
    assert(foo != NULL);
    hello = android_dlsym(foo, "hello");
    assert(hello != NULL);
    (*hello)('z', 'd');
    printf("full stop\n");    
exit(0); 
}  
