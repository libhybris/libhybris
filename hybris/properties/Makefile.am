lib_LTLIBRARIES = \
	libandroid-properties.la

libandroid_properties_la_SOURCES = properties.c cache.c
libandroid_properties_la_CFLAGS = -I$(top_srcdir)/include
if WANT_DEBUG
libandroid_properties_la_CFLAGS += -ggdb -O0
endif
libandroid_properties_la_LDFLAGS = \
	-version-info "1":"0":"0"

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libandroid-properties.pc
