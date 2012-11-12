lib_LTLIBRARIES = \
	libEGL.la

libEGL_la_SOURCES = egl.c
libEGL_la_CFLAGS = -I$(top_srcdir)/include
if WANT_DEBUG
libEGL_la_CFLAGS += -ggdb -O0 -DDEBUG
endif
libEGL_la_LDFLAGS = \
	$(top_builddir)/common/libhybris-common.la \
	-version-info "1":"0":"0"
