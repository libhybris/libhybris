lib_LTLIBRARIES = \
	libhybris-eglplatformcommon.la

libhybris_eglplatformcommon_la_SOURCES = \
	native_handle.c \
	nativewindowbase.cpp

libhybris_eglplatformcommon_la_CFLAGS = -I$(top_srcdir)/include
if WANT_MESA
libhybris_eglplatformcommon_la_CFLAGS += -DLIBHYBRIS_WANTS_MESA_X11_HEADERS
endif
if WANT_DEBUG
libhybris_eglplatformcommon_la_CFLAGS += -ggdb -O0 -DDEBUG
endif
libhybris_eglplatformcommon_la_CXXFLAGS = -I$(top_srcdir)/include
if WANT_MESA
libhybris_eglplatformcommon_la_CXXFLAGS += -DLIBHYBRIS_WANTS_MESA_X11_HEADERS
endif
if WANT_DEBUG
libhybris_eglplatformcommon_la_CXXFLAGS += -ggdb -O0 -DDEBUG
endif
libhybris_eglplatformcommon_la_LDFLAGS = \
	$(top_builddir)/common/libhybris-common.la \
	-version-info "1":"0":"0"

