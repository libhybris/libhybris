if HAS_VULKAN_HEADERS

SUBDIRS = platforms

lib_LTLIBRARIES = \
	libvulkan.la

libvulkan_la_SOURCES = \
	vulkan.c \
	ws.c

libvulkan_la_CFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/common $(ANDROID_HEADERS_CFLAGS) -DPKGLIBDIR="\"$(pkglibdir)/\""
if WANT_TRACE
libvulkan_la_CFLAGS += -DDEBUG
endif
if WANT_DEBUG
libvulkan_la_CFLAGS += -ggdb -O0
endif

libvulkan_la_CXXFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/common $(ANDROID_HEADERS_CFLAGS) -DPKGLIBDIR="\"$(pkglibdir)/\""
if WANT_TRACE
libvulkan_la_CXXFLAGS += -DDEBUG
endif
if WANT_DEBUG
libvulkan_la_CXXFLAGS += -ggdb -O0
endif
libvulkan_la_LDFLAGS = \
	-ldl \
	$(top_builddir)/common/libhybris-common.la \
	$(top_builddir)/hardware/libhardware.la \
	-version-info 3:183:2

endif
