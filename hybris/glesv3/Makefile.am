lib_LTLIBRARIES = \
	libGLESv3.la

libGLESv3_la_SOURCES = glesv3.c
libGLESv3_la_CFLAGS = -I$(top_srcdir)/include $(ANDROID_HEADERS_CFLAGS)

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = glesv3.pc

if WANT_MESA
libGLESv3_la_CFLAGS += -DLIBHYBRIS_WANTS_MESA_X11_HEADERS
endif
if WANT_TRACE
libGLESv3_la_CFLAGS += -DDEBUG
endif
if WANT_DEBUG
libGLESv3_la_CFLAGS += -ggdb -O0
endif
libGLESv3_la_LDFLAGS = \
	$(top_builddir)/common/libhybris-common.la \
	-version-info "3":"0":"0"
