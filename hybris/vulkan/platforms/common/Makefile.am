lib_LTLIBRARIES = \
	libhybris-vulkanplatformcommon.la

libhybris_vulkanplatformcommon_la_SOURCES = \
	vulkanplatformcommon.cpp

libhybris_vulkanplatformcommon_la_CFLAGS = -I$(top_srcdir)/include $(ANDROID_HEADERS_CFLAGS) -I$(top_srcdir)/vulkan -I$(top_srcdir)/common/ -I$(top_srcdir)/platforms/common/
if WANT_WAYLAND
libhybris_vulkanplatformcommon_la_CFLAGS += $(WAYLAND_CLIENT_CFLAGS) $(WAYLAND_SERVER_CFLAGS) $(ANDROID_HEADERS_CFLAGS)
endif

if WANT_TRACE
libhybris_vulkanplatformcommon_la_CFLAGS += -DDEBUG
endif
if WANT_DEBUG
libhybris_vulkanplatformcommon_la_CFLAGS += -ggdb -O0
endif

libhybris_vulkanplatformcommon_la_CXXFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/vulkan $(ANDROID_HEADERS_CFLAGS) -I$(top_srcdir)/common/ -I$(top_srcdir)/platforms/common/

if WANT_WAYLAND
libhybris_vulkanplatformcommon_la_CXXFLAGS += $(WAYLAND_CLIENT_CFLAGS) $(WAYLAND_SERVER_CFLAGS) $(ANDROID_HEADERS_CFLAGS)
endif

if WANT_TRACE
libhybris_vulkanplatformcommon_la_CXXFLAGS += -DDEBUG
endif
if WANT_DEBUG
libhybris_vulkanplatformcommon_la_CXXFLAGS += -ggdb -O0
endif
libhybris_vulkanplatformcommon_la_LDFLAGS = \
	$(top_builddir)/common/libhybris-common.la \
	$(top_builddir)/platforms/common/libhybris-platformcommon.la \
	-version-info 1:0:0

if HAS_ANDROID_5_0_0
libhybris_vulkanplatformcommon_la_LDFLAGS += $(top_builddir)/libsync/libsync.la
endif

if WANT_WAYLAND
libhybris_vulkanplatformcommon_la_LDFLAGS += \
	$(WAYLAND_CLIENT_LIBS) \
	$(WAYLAND_SERVER_LIBS)
endif
