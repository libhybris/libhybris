lib_LTLIBRARIES = \
	libhybris-common.la

if  WANT_GB
SUBDIRS = gingerbread
libhybris_common_la_LIBADD= gingerbread/libandroid-linker.la
endif

if  WANT_ICS
SUBDIRS = ics
libhybris_common_la_LIBADD= ics/libandroid-linker.la
endif

if  WANT_JB
SUBDIRS = jb
libhybris_common_la_LIBADD= jb/libandroid-linker.la
endif


libhybris_common_la_SOURCES = \
	hooks.c \
	hooks_shm.c \
	properties.c \
	strlcpy.c \
	logging.c
libhybris_common_la_CFLAGS = \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/common
if WANT_TRACE
libhybris_common_la_CFLAGS += -DDEBUG
endif
if WANT_DEBUG
libhybris_common_la_CFLAGS += -ggdb -O0
endif
libhybris_common_la_LDFLAGS = \
	-ldl \
	-lrt \
	-pthread \
	-version-info "$(LT_CURRENT)":"$(LT_REVISION)":"$(LT_AGE)"
