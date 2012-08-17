#define MESA_EGL_NO_X11_HEADERS
/* EGL function pointers */
#include <EGL/egl.h>
#include <dlfcn.h>
#include <stddef.h>

static void *_libegl = NULL;
static void *_libui = NULL;

static EGLint  (*_eglGetError)(void) = NULL;

static EGLDisplay  (*_eglGetDisplay)(EGLNativeDisplayType display_id) = NULL;
static EGLBoolean  (*_eglInitialize)(EGLDisplay dpy, EGLint *major, EGLint *minor) = NULL;
static EGLBoolean  (*_eglTerminate)(EGLDisplay dpy) = NULL;

static const char *  (*_eglQueryString)(EGLDisplay dpy, EGLint name) = NULL;

static EGLBoolean  (*_eglGetConfigs)(EGLDisplay dpy, EGLConfig *configs,
			 EGLint config_size, EGLint *num_config) = NULL;
static EGLBoolean  (*_eglChooseConfig)(EGLDisplay dpy, const EGLint *attrib_list,
			   EGLConfig *configs, EGLint config_size,
			   EGLint *num_config) = NULL;
static EGLBoolean  (*_eglGetConfigAttrib)(EGLDisplay dpy, EGLConfig config,
			      EGLint attribute, EGLint *value) = NULL;

static EGLSurface  (*_eglCreateWindowSurface)(EGLDisplay dpy, EGLConfig config,
				  EGLNativeWindowType win,
				  const EGLint *attrib_list) = NULL;
static EGLSurface  (*_eglCreatePbufferSurface)(EGLDisplay dpy, EGLConfig config,
				   const EGLint *attrib_list) = NULL;
static EGLSurface  (*_eglCreatePixmapSurface)(EGLDisplay dpy, EGLConfig config,
				  EGLNativePixmapType pixmap,
				  const EGLint *attrib_list) = NULL;
static EGLBoolean  (*_eglDestroySurface)(EGLDisplay dpy, EGLSurface surface) = NULL;
static EGLBoolean  (*_eglQuerySurface)(EGLDisplay dpy, EGLSurface surface,
			   EGLint attribute, EGLint *value) = NULL;

static EGLBoolean  (*_eglBindAPI)(EGLenum api) = NULL;
static EGLenum  (*_eglQueryAPI)(void) = NULL;

static EGLBoolean  (*_eglWaitClient)(void) = NULL;

static EGLBoolean  (*_eglReleaseThread)(void) = NULL;

static EGLSurface  (*_eglCreatePbufferFromClientBuffer)(
	      EGLDisplay dpy, EGLenum buftype, EGLClientBuffer buffer,
	      EGLConfig config, const EGLint *attrib_list) = NULL;

static EGLBoolean  (*_eglSurfaceAttrib)(EGLDisplay dpy, EGLSurface surface,
			    EGLint attribute, EGLint value) = NULL;
static EGLBoolean  (*_eglBindTexImage)(EGLDisplay dpy, EGLSurface surface, EGLint buffer) = NULL;
static EGLBoolean  (*_eglReleaseTexImage)(EGLDisplay dpy, EGLSurface surface, EGLint buffer) = NULL;


static EGLBoolean  (*_eglSwapInterval)(EGLDisplay dpy, EGLint interval) = NULL;


static EGLContext  (*_eglCreateContext)(EGLDisplay dpy, EGLConfig config,
			    EGLContext share_context,
			    const EGLint *attrib_list) = NULL;
static EGLBoolean  (*_eglDestroyContext)(EGLDisplay dpy, EGLContext ctx) = NULL;
static EGLBoolean  (*_eglMakeCurrent)(EGLDisplay dpy, EGLSurface draw,
			  EGLSurface read, EGLContext ctx) = NULL;

static EGLContext  (*_eglGetCurrentContext)(void) = NULL;
static EGLSurface  (*_eglGetCurrentSurface)(EGLint readdraw) = NULL;
static EGLDisplay  (*_eglGetCurrentDisplay)(void) = NULL;
static EGLBoolean  (*_eglQueryContext)(EGLDisplay dpy, EGLContext ctx,
			   EGLint attribute, EGLint *value) = NULL;

static EGLBoolean  (*_eglWaitGL)(void) = NULL;
static EGLBoolean  (*_eglWaitNative)(EGLint engine) = NULL;
static EGLBoolean  (*_eglSwapBuffers)(EGLDisplay dpy, EGLSurface surface) = NULL;
static EGLBoolean  (*_eglCopyBuffers)(EGLDisplay dpy, EGLSurface surface,
			  EGLNativePixmapType target) = NULL;

static __eglMustCastToProperFunctionPointerType (*_eglGetProcAddress)(const char *procname);

static void * (*_androidCreateDisplaySurface)();

static void _init_androidegl()
{
 _libegl = (void *) android_dlopen("/system/lib/libEGL.so", RTLD_LAZY);
}

static void _init_androidui()
{
 _libui = (void *) android_dlopen("/system/lib/libui.so", RTLD_LAZY);
}

#define EGL_DLSYM(fptr, sym) do { if (_libegl == NULL) { _init_androidegl(); }; if (*(fptr) == NULL) { *(fptr) = (void *) android_dlsym(_libegl, sym); } } while (0) 

#define UI_DLSYM(fptr, sym) do { if (_libui == NULL) { _init_androidui(); }; if (*(fptr) == NULL) { *(fptr) = (void *) android_dlsym(_libui, sym); } } while (0) 

EGLint eglGetError(void)
{
 EGL_DLSYM(&_eglGetError, "eglGetError");
 return (*_eglGetError)();
}


EGLDisplay eglGetDisplay(EGLNativeDisplayType display_id)
{
 EGL_DLSYM(&_eglGetDisplay, "eglGetDisplay");
 return (*_eglGetDisplay)(display_id);
}

EGLBoolean eglInitialize(EGLDisplay dpy, EGLint *major, EGLint *minor)
{
 EGL_DLSYM(&_eglInitialize, "eglInitialize");
 return (*_eglInitialize)(dpy, major, minor);
}

EGLBoolean eglTerminate(EGLDisplay dpy)
{
 EGL_DLSYM(&_eglTerminate, "eglTerminate");
 return (*_eglTerminate)(dpy);
}


const char * eglQueryString(EGLDisplay dpy, EGLint name)
{
 EGL_DLSYM(&_eglQueryString, "eglQueryString");
 return (*_eglQueryString)(dpy, name);
}


EGLBoolean eglGetConfigs(EGLDisplay dpy, EGLConfig *configs,
			 EGLint config_size, EGLint *num_config)
{
 EGL_DLSYM(&_eglGetConfigs, "eglGetConfigs");
 return (*_eglGetConfigs)(dpy, configs, config_size, num_config);
}

EGLBoolean eglChooseConfig(EGLDisplay dpy, const EGLint *attrib_list,
			   EGLConfig *configs, EGLint config_size,
			   EGLint *num_config)
{
 EGL_DLSYM(&_eglChooseConfig, "eglChooseConfig");
 return (*_eglChooseConfig)(dpy, attrib_list,
			   configs, config_size,
			   num_config);
}

EGLBoolean eglGetConfigAttrib(EGLDisplay dpy, EGLConfig config,
			      EGLint attribute, EGLint *value)
{
 EGL_DLSYM(&_eglGetConfigAttrib, "eglGetConfigAttrib");
 return (*_eglGetConfigAttrib)(dpy, config,
			      attribute, value);
}


EGLSurface eglCreateWindowSurface(EGLDisplay dpy, EGLConfig config,
				  EGLNativeWindowType win,
				  const EGLint *attrib_list)
{
 EGL_DLSYM(&_eglCreateWindowSurface, "eglCreateWindowSurface");
 UI_DLSYM(&_androidCreateDisplaySurface, "android_createDisplaySurface");
 if (win == 0)
 {
     win = (EGLNativeWindowType) (*_androidCreateDisplaySurface)();
 } 
 return (*_eglCreateWindowSurface)(dpy, config,
				  win,
				  attrib_list);
}

EGLSurface eglCreatePbufferSurface(EGLDisplay dpy, EGLConfig config,
				   const EGLint *attrib_list)
{
 EGL_DLSYM(&_eglCreatePbufferSurface, "eglCreatePbufferSurface");
 return (*_eglCreatePbufferSurface)(dpy, config,
				   attrib_list);
}

EGLSurface eglCreatePixmapSurface(EGLDisplay dpy, EGLConfig config,
				  EGLNativePixmapType pixmap,
				  const EGLint *attrib_list)
{
 EGL_DLSYM(&_eglCreatePixmapSurface, "eglCreatePixmapSurface");
 return (*_eglCreatePixmapSurface)(dpy, config,
				  pixmap,
				  attrib_list);
}

EGLBoolean eglDestroySurface(EGLDisplay dpy, EGLSurface surface)
{
 EGL_DLSYM(&_eglDestroySurface, "eglDestroySurface");
 return (*_eglDestroySurface)(dpy, surface);
}

EGLBoolean eglQuerySurface(EGLDisplay dpy, EGLSurface surface,
			   EGLint attribute, EGLint *value)
{
 EGL_DLSYM(&_eglQuerySurface, "eglQuerySurface");
 return (*_eglQuerySurface)(dpy, surface,
			   attribute, value);
}


EGLBoolean eglBindAPI(EGLenum api)
{
 EGL_DLSYM(&_eglBindAPI, "eglBindAPI");
 return (*_eglBindAPI)(api);
}

EGLenum eglQueryAPI(void)
{
 EGL_DLSYM(&_eglQueryAPI, "eglQueryAPI");
 return (*_eglQueryAPI)();
}


EGLBoolean eglWaitClient(void)
{
 EGL_DLSYM(&_eglWaitClient, "eglWaitClient");
 return (*_eglWaitClient)();
}


EGLBoolean eglReleaseThread(void)
{
 EGL_DLSYM(&_eglReleaseThread, "eglReleaseThread");
 return (*_eglReleaseThread)();
}


EGLSurface eglCreatePbufferFromClientBuffer(
	      EGLDisplay dpy, EGLenum buftype, EGLClientBuffer buffer,
	      EGLConfig config, const EGLint *attrib_list)
{
 EGL_DLSYM(&_eglCreatePbufferFromClientBuffer, "eglCreatePbufferFromClientBuffer");
 return (*_eglCreatePbufferFromClientBuffer)(
	      dpy, buftype, buffer,
	      config, attrib_list);

}


EGLBoolean eglSurfaceAttrib(EGLDisplay dpy, EGLSurface surface,
			    EGLint attribute, EGLint value)
{
 EGL_DLSYM(&_eglSurfaceAttrib, "eglSurfaceAttrib");
 return (*_eglSurfaceAttrib)(dpy, surface,
 			    attribute, value);
}

EGLBoolean eglBindTexImage(EGLDisplay dpy, EGLSurface surface, EGLint buffer)
{
 EGL_DLSYM(&_eglBindTexImage, "eglBindTexImage");
 return (*_eglBindTexImage)(dpy, surface, buffer);
}

EGLBoolean eglReleaseTexImage(EGLDisplay dpy, EGLSurface surface, EGLint buffer)
{
 EGL_DLSYM(&_eglReleaseTexImage, "eglReleaseTexImage");
 return (*_eglReleaseTexImage)(dpy, surface, buffer);
}



EGLBoolean eglSwapInterval(EGLDisplay dpy, EGLint interval)
{
 EGL_DLSYM(&_eglSwapInterval, "eglSwapInterval");
 return (*_eglSwapInterval)(dpy, interval);
}



EGLContext eglCreateContext(EGLDisplay dpy, EGLConfig config,
			    EGLContext share_context,
			    const EGLint *attrib_list)
{
 EGL_DLSYM(&_eglCreateContext, "eglCreateContext");
 return (*_eglCreateContext)(dpy, config,
			    share_context,
			    attrib_list);
}

EGLBoolean eglDestroyContext(EGLDisplay dpy, EGLContext ctx)
{
 EGL_DLSYM(&_eglDestroyContext, "eglDestroyContext");
 return (*_eglDestroyContext)(dpy, ctx);
}

EGLBoolean eglMakeCurrent(EGLDisplay dpy, EGLSurface draw,
			  EGLSurface read, EGLContext ctx)
{
 EGL_DLSYM(&_eglMakeCurrent, "eglMakeCurrent");
 return (*_eglMakeCurrent)(dpy, draw,
			  read, ctx);

}


EGLContext eglGetCurrentContext(void)
{
 EGL_DLSYM(&_eglGetCurrentContext, "eglGetCurrentContext");
 return (*_eglGetCurrentContext)();
}

EGLSurface eglGetCurrentSurface(EGLint readdraw)
{
 EGL_DLSYM(&_eglGetCurrentSurface, "eglGetCurrentSurface");
 return (*_eglGetCurrentSurface)(readdraw);
}

EGLDisplay eglGetCurrentDisplay(void)
{
 EGL_DLSYM(&_eglGetCurrentDisplay, "eglGetCurrentDisplay");
 return (*_eglGetCurrentDisplay)();
}

EGLBoolean eglQueryContext(EGLDisplay dpy, EGLContext ctx,
			   EGLint attribute, EGLint *value)
{
 EGL_DLSYM(&_eglQueryContext, "eglQueryContext");
 return (*_eglQueryContext)(dpy, ctx,
			   attribute, value);
}


EGLBoolean eglWaitGL(void)
{
 EGL_DLSYM(&_eglWaitGL, "eglWaitGL");
 return (*_eglWaitGL)();
}

EGLBoolean eglWaitNative(EGLint engine)
{
 EGL_DLSYM(&_eglWaitNative, "eglWaitNative");
 return (*_eglWaitNative)(engine); 
}

EGLBoolean eglSwapBuffers(EGLDisplay dpy, EGLSurface surface)
{
 EGL_DLSYM(&_eglSwapBuffers, "eglSwapBuffers");
 return (*_eglSwapBuffers)(dpy, surface);
}

EGLBoolean eglCopyBuffers(EGLDisplay dpy, EGLSurface surface,
			  EGLNativePixmapType target)
{
 EGL_DLSYM(&_eglCopyBuffers, "eglCopyBuffers");
 return (*_eglCopyBuffers)(dpy, surface,
                        target);
}

__eglMustCastToProperFunctionPointerType eglGetProcAddress(const char *procname)
{
 EGL_DLSYM(&_eglGetProcAddress, "eglGetProcAddress");
 return (*_eglGetProcAddress)(procname);
}
