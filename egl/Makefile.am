lib_LTLIBRARIES = \
	libEGL.la

libEGL_la_SOURCES = egl.c ws.c ws_fbdev.cpp nativewindowbase.cpp fbdev_window.cpp
libEGL_la_CFLAGS = -I$(top_srcdir)/include
if WANT_DEBUG
libEGL_la_CFLAGS += -ggdb -O0 -DDEBUG
endif
libEGL_la_CXXFLAGS = -I$(top_srcdir)/include
if WANT_DEBUG
libEGL_la_CXXFLAGS += -ggdb -O0 -DDEBUG
endif
libEGL_la_LDFLAGS = \
	$(top_builddir)/common/libhybris-common.la \
	$(top_builddir)/hardware/libhardware.la \
	-version-info "1":"0":"0"
