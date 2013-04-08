lib_LTLIBRARIES = \
	libEGL.la

libEGL_la_SOURCES = \
	egl.c \
	ws.c \
	ws_fbdev.cpp \
	nativewindowbase.cpp \
	fbdev_window.cpp \
	fdpass.c \
	native_handle.c \
	offscreen_window.cpp \
	offscreen_window_buffer.cpp
libEGL_la_CFLAGS = -I$(top_srcdir)/include
if WANT_MESA
libEGL_la_CFLAGS += -DLIBHYBRIS_WANTS_MESA_X11_HEADERS
endif
if WANT_DEBUG
libEGL_la_CFLAGS += -ggdb -O0 -DDEBUG
endif
libEGL_la_CXXFLAGS = -I$(top_srcdir)/include
if WANT_MESA
libEGL_la_CXXFLAGS += -DLIBHYBRIS_WANTS_MESA_X11_HEADERS
endif
if WANT_DEBUG
libEGL_la_CXXFLAGS += -ggdb -O0 -DDEBUG
endif
libEGL_la_LDFLAGS = \
	$(top_builddir)/common/libhybris-common.la \
	$(top_builddir)/hardware/libhardware.la \
	-version-info "1":"0":"0"

eglincludedir = $(includedir)/EGL
eglinclude_HEADERS = \
	eglhybris.h \
	offscreen_window.h \
	fbdev_window.h \
	nativewindowbase.h \
	support.h
