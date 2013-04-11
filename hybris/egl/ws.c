

#include <ws.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <assert.h>
static struct ws_module *ws = NULL;

static void _init_ws()
{
	if (ws == NULL)
	{
		char *egl_platform = getenv("EGL_PLATFORM");
		char ws_name[2048];
		
		if (egl_platform == NULL)
			egl_platform = "null";
	
		snprintf(ws_name, 2048, PKGLIBDIR "eglplatform_%s.so", egl_platform);	
			
		void *wsmod = (void *) dlopen(ws_name, RTLD_LAZY);
		assert(wsmod != NULL);
		ws = dlsym(wsmod, "ws_module_info");
		assert(ws != NULL);
	}
}


int ws_IsValidDisplay(EGLNativeDisplayType display)
{
	_init_ws();
	return ws->IsValidDisplay(display);
}

EGLNativeWindowType ws_CreateWindow(EGLNativeWindowType win, EGLNativeDisplayType display)
{
	_init_ws();
	return ws->CreateWindow(win, display);
}

__eglMustCastToProperFunctionPointerType ws_eglGetProcAddress(const char *procname) 
{
	_init_ws();
	return ws->eglGetProcAddress(procname);
}

void ws_passthroughImageKHR(EGLenum *target, EGLClientBuffer *buffer)
{
	_init_ws();
	return ws->passthroughImageKHR(target, buffer);
}

