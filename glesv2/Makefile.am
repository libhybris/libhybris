lib_LTLIBRARIES = \
	libGLESv2.la

libGLESv2_la_SOURCES = glesv2.c
libGLESv2_la_CFLAGS = -I$(top_srcdir)/include
if WANT_DEBUG
libGLESv2_la_CFLAGS += -ggdb -O0 -DDEBUG
endif
libGLESv2_la_LDFLAGS = \
	$(top_builddir)/common/libhybris-common.la \
	-version-info "2":"0":"0"
