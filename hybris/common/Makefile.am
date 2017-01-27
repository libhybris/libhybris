lib_LTLIBRARIES = \
	libhybris-common.la

if HAS_ANDROID_6_0_0
SUBDIRS = mm
libhybris_common_la_LIBADD = mm/libandroid-linker.la
else
# JB linker works fine for ICS
if HAS_ANDROID_4_0_0
SUBDIRS = jb
libhybris_common_la_LIBADD= jb/libandroid-linker.la
else
if HAS_ANDROID_5_0_0
SUBDIRS = jb
libhybris_common_la_LIBADD= jb/libandroid-linker.la
else
if HAS_ANDROID_2_3_0
SUBDIRS = gingerbread
libhybris_common_la_LIBADD= gingerbread/libandroid-linker.la
else
$(error No Android Version is defined)
endif
endif
endif
endif

libhybris_common_la_SOURCES = \
	hooks.c \
	hooks_shm.c \
	strlcpy.c \
	strlcat.c \
	dlfcn.c \
	logging.c \
	sysconf.c
libhybris_common_la_CFLAGS = \
	-I$(top_srcdir)/include \
	$(ANDROID_HEADERS_CFLAGS) \
	-I$(top_srcdir)/common

if HAS_ANDROID_6_0_0
# force c++ linker
nodist_EXTRA_libhybris_common_la_SOURCES = dummy.cpp
endif

if WANT_TRACE
libhybris_common_la_CFLAGS += -DDEBUG
endif
if WANT_DEBUG
libhybris_common_la_CFLAGS += -ggdb -O0
endif
libhybris_common_la_LDFLAGS = \
	-ldl \
	-lrt \
	-lpthread \
	$(top_builddir)/properties/libandroid-properties.la \
	-version-info "$(LT_CURRENT)":"$(LT_REVISION)":"$(LT_AGE)"
