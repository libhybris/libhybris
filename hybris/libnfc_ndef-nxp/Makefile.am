lib_LTLIBRARIES = \
	libnfc_ndef-nxp.la

libnfc_ndef-nxp_la_SOURCES = libnfc_ndef-nxp.c
libnfc_ndef-nxp_la_CFLAGS = -I$(top_srcdir)/include

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libnfc_ndef-nxp.pc

if WANT_TRACE
libnfc_ndef-nxp_la_CFLAGS += -DDEBUG
endif

if WANT_DEBUG
libnfc_ndef-nxp_la_CFLAGS += -ggdb -O0
endif

libnfc_ndef-nxp_la_LDFLAGS = \
	$(top_builddir)/common/libhybris-common.la \
	-version-info "0":"0":"1"
