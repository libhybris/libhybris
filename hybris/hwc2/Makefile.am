lib_LTLIBRARIES = \
	libhwc2.la

libhwc2_la_SOURCES = hwc2.c
libhwc2_la_CFLAGS = -I$(top_srcdir)/include $(ANDROID_HEADERS_CFLAGS)
if WANT_TRACE
libhwc2_la_CFLAGS += -DDEBUG
endif
if WANT_DEBUG
libhwc2_la_CFLAGS += -ggdb -O0
endif
libhwc2_la_LDFLAGS = \
	$(top_builddir)/common/libhybris-common.la \
	-version-info "1":"0":"0"

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libhwc2.pc
