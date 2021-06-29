lib_LTLIBRARIES = \
	libGLESv1_CM$(GL_LIB_SUFFIX).la

libGLESv1_CM__GL_LIB_SUFFIX__la_SOURCES = glesv1_cm.c
libGLESv1_CM__GL_LIB_SUFFIX__la_CFLAGS = -I$(top_srcdir)/include $(ANDROID_HEADERS_CFLAGS)

if !WANT_GLVND
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = glesv1_cm.pc
endif

if WANT_MESA
libGLESv1_CM__GL_LIB_SUFFIX__la_CFLAGS += -DLIBHYBRIS_WANTS_MESA_X11_HEADERS
endif
if WANT_TRACE
libGLESv1_CM__GL_LIB_SUFFIX__la_CFLAGS += -DDEBUG
endif
if WANT_DEBUG
libGLESv1_CM__GL_LIB_SUFFIX__la_CFLAGS += -ggdb -O0
endif
libGLESv1_CM__GL_LIB_SUFFIX__la_LDFLAGS = \
	$(top_builddir)/common/libhybris-common.la \
	-version-info "1":"1":"0"
