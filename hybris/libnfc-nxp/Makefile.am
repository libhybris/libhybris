lib_LTLIBRARIES = \
	libnfc-nxp.la

libnfc-nxp_la_SOURCES = libnfc-nxp.c
libnfc-nxp_la_CFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/include/android

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libnfc-nxp.pc

if WANT_TRACE
libnfc-nxp_la_CFLAGS += -DDEBUG
endif

if WANT_DEBUG
libnfc-nxp_la_CFLAGS += -ggdb -O0
endif

libnfc-nxp_la_LDFLAGS = \
	$(top_builddir)/common/libhybris-common.la \
	-version-info "0":"0":"1"
