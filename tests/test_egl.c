#define MESA_EGL_NO_X11_HEADERS

#include <EGL/egl.h>
#include <assert.h>
#include <stdio.h>

int main(int argc, char **argv)
{
	EGLDisplay display;
	EGLConfig ecfg;
	EGLint num_config;
	EGLint attr[] = {       // some attributes to set up our egl-interface
		EGL_BUFFER_SIZE, 32,
		EGL_RENDERABLE_TYPE,
		EGL_OPENGL_ES2_BIT,
		EGL_NONE
	};
	EGLSurface surface;
	EGLint ctxattr[] = {
		EGL_CONTEXT_CLIENT_VERSION, 2,
		EGL_NONE
	};
	EGLContext context;

	display = eglGetDisplay(NULL);

	eglInitialize(display, 0, 0);
        assert(eglChooseConfig((EGLDisplay) display, attr, &ecfg, 1, &num_config) == EGL_TRUE);
	surface = eglCreateWindowSurface((EGLDisplay) display, ecfg, (EGLNativeWindowType)NULL, NULL);
	assert(surface != EGL_NO_SURFACE);
	context = eglCreateContext((EGLDisplay) display, ecfg, EGL_NO_CONTEXT, ctxattr);
        assert(surface != EGL_NO_CONTEXT);
	assert(eglMakeCurrent((EGLDisplay) display, surface, surface, context) == EGL_TRUE);
	printf("stop\n");

#if 0
(*egldestroycontext)((EGLDisplay) display, context);
    printf("destroyed context\n");

    (*egldestroysurface)((EGLDisplay) display, surface);
    printf("destroyed surface\n");
    (*eglterminate)((EGLDisplay) display);
    printf("terminated\n");
    android_dlclose(baz);
#endif
}
