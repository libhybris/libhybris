pkglib_LTLIBRARIES = vulkanplatform_wayland.la

vulkanplatform_wayland_la_SOURCES = vulkanplatform_wayland.cpp wayland_window.cpp $(top_srcdir)/platforms/wayland/wayland_window_common.cpp
vulkanplatform_wayland_la_CXXFLAGS = \
	-I$(top_srcdir)/common \
	-I$(top_srcdir)/platforms/common \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/vulkan \
	-I$(top_srcdir)/vulkan/platforms/common \
	-I$(top_builddir)/vulkan/platforms/common \
	$(ANDROID_HEADERS_CFLAGS) \
	$(WAYLAND_CLIENT_CFLAGS) \
	$(WAYLAND_EGL_CFLAGS)

if WANT_DEBUG
vulkanplatform_wayland_la_CXXFLAGS += -I$(top_srcdir)/common
endif

if WANT_TRACE
vulkanplatform_wayland_la_CXXFLAGS += -DDEBUG
endif

if WANT_DEBUG
vulkanplatform_wayland_la_CXXFLAGS += -ggdb -O0
endif

if !WANT_WL_SERVERSIDE_BUFFERS
vulkanplatform_wayland_la_CXXFLAGS += -DHYBRIS_NO_SERVER_SIDE_BUFFERS
endif

vulkanplatform_wayland_la_LDFLAGS = \
	-avoid-version -module -shared -export-dynamic \
	$(top_builddir)/vulkan/platforms/common/libhybris-vulkanplatformcommon.la \
	$(top_builddir)/gralloc/libgralloc.la \
	$(WAYLAND_CLIENT_LIBS) \
	$(WAYLAND_EGL_LIBS)

if HAS_ANDROID_4_2_0
vulkanplatform_wayland_la_LDFLAGS += $(top_builddir)/libsync/libsync.la
endif

if HAS_ANDROID_5_0_0
vulkanplatform_wayland_la_LDFLAGS += $(top_builddir)/libsync/libsync.la
endif

