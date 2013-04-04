lib_LTLIBRARIES = \
	libhardware.la

libhardware_la_SOURCES = hardware.c
libhardware_la_CFLAGS = -I$(top_srcdir)/include
if WANT_DEBUG
libhardware_la_CFLAGS += -ggdb -O0 -DDEBUG
endif
libhardware_la_LDFLAGS = \
	$(top_builddir)/common/libhybris-common.la \
	-version-info "2":"0":"0"
