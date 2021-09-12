pkglib_LTLIBRARIES = vulkanplatform_null.la

vulkanplatform_null_la_SOURCES = vulkanplatform_null.c
vulkanplatform_null_la_CFLAGS = \
	-I$(top_srcdir)/common \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/vulkan \
	-I$(top_srcdir)/vulkan/platforms/common \
	-I$(top_builddir)/vulkan/platforms/common \
	$(ANDROID_HEADERS_CFLAGS) \
	$(WAYLAND_SERVER_CFLAGS)

vulkanplatform_null_la_LDFLAGS = \
	-avoid-version -module -shared -export-dynamic \
	$(top_builddir)/vulkan/platforms/common/libhybris-vulkanplatformcommon.la \
	$(top_builddir)/gralloc/libgralloc.la \
	$(WAYLAND_SERVER_LIBS)

if WANT_DEBUG
vulkanplatform_null_la_CFLAGS += -I$(top_srcdir)/common -ggdb -O0
endif

if WANT_TRACE
vulkanplatform_null_la_CFLAGS += -DDEBUG
endif
