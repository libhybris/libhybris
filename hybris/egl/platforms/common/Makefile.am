lib_LTLIBRARIES = \
	libhybris-eglplatformcommon.la

libhybris_eglplatformcommon_la_SOURCES = \
	eglplatformcommon.cpp

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = hybris-egl-platform.pc

libhybris_eglplatformcommon_la_CFLAGS = -I$(top_srcdir)/include $(ANDROID_HEADERS_CFLAGS) -I$(top_srcdir)/egl -I$(top_srcdir)/common/ -I$(top_srcdir)/platforms/common/
if WANT_WAYLAND
libhybris_eglplatformcommon_la_CFLAGS += $(WAYLAND_CLIENT_CFLAGS) $(WAYLAND_SERVER_CFLAGS) $(ANDROID_HEADERS_CFLAGS)
endif

if WANT_MESA
libhybris_eglplatformcommon_la_CFLAGS += -DLIBHYBRIS_WANTS_MESA_X11_HEADERS
endif
if WANT_TRACE
libhybris_eglplatformcommon_la_CFLAGS += -DDEBUG
endif
if WANT_DEBUG
libhybris_eglplatformcommon_la_CFLAGS += -ggdb -O0
endif


libhybris_eglplatformcommon_la_CXXFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/egl $(ANDROID_HEADERS_CFLAGS) -I$(top_srcdir)/common/ -I$(top_srcdir)/platforms/common/
if WANT_MESA
libhybris_eglplatformcommon_la_CXXFLAGS += -DLIBHYBRIS_WANTS_MESA_X11_HEADERS
endif

if WANT_WAYLAND
libhybris_eglplatformcommon_la_CXXFLAGS += $(WAYLAND_CLIENT_CFLAGS) $(WAYLAND_SERVER_CFLAGS) $(ANDROID_HEADERS_CFLAGS)
endif

if WANT_TRACE
libhybris_eglplatformcommon_la_CXXFLAGS += -DDEBUG
endif
if WANT_DEBUG
libhybris_eglplatformcommon_la_CXXFLAGS += -ggdb -O0
endif
libhybris_eglplatformcommon_la_LDFLAGS = \
	$(top_builddir)/common/libhybris-common.la \
	$(top_builddir)/platforms/common/libhybris-platformcommon.la \
	-version-info "1":"0":"0"

if HAS_ANDROID_4_2_0
libhybris_eglplatformcommon_la_LDFLAGS += $(top_builddir)/libsync/libsync.la
endif

if HAS_ANDROID_5_0_0
libhybris_eglplatformcommon_la_LDFLAGS += $(top_builddir)/libsync/libsync.la
endif


eglplatformcommondir = $(includedir)/hybris/eglplatformcommon
eglplatformcommon_HEADERS = \
	eglplatformcommon.h \
	eglnativewindowbase.h \
	hybris_nativebufferext.h

if WANT_WAYLAND
libhybris_eglplatformcommon_la_LDFLAGS += \
	$(WAYLAND_CLIENT_LIBS) \
	$(WAYLAND_SERVER_LIBS)
endif
