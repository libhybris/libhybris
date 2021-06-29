lib_LTLIBRARIES = \
	libGLESv2$(GL_LIB_SUFFIX).la

libGLESv2__GL_LIB_SUFFIX__la_SOURCES = glesv2.c
libGLESv2__GL_LIB_SUFFIX__la_CFLAGS = -I$(top_srcdir) -I$(top_srcdir)/include $(ANDROID_HEADERS_CFLAGS)

if !WANT_GLVND
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = glesv2.pc
endif

if WANT_MESA
libGLESv2__GL_LIB_SUFFIX__la_CFLAGS += -DLIBHYBRIS_WANTS_MESA_X11_HEADERS
endif
if WANT_TRACE
libGLESv2__GL_LIB_SUFFIX__la_CFLAGS += -DDEBUG
endif
if WANT_DEBUG
libGLESv2__GL_LIB_SUFFIX__la_CFLAGS += -ggdb -O0
endif
libGLESv2__GL_LIB_SUFFIX__la_LDFLAGS = \
	$(top_builddir)/common/libhybris-common.la \
	-version-info "2":"0":"0"
