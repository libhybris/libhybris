lib_LTLIBRARIES = \
	libhybris-common.la

SUBDIRS =

if !WANT_ARCH_ARM64
SUBDIRS += jb
endif

if WANT_EXPERIMENTAL
SUBDIRS += mm
endif

libhybris_common_la_SOURCES = \
	hooks.c \
	hooks_shm.c \
	strlcpy.c \
	strlcat.c \
	logging.c \
	sysconf.c
libhybris_common_la_CPPFLAGS = \
	-I$(top_srcdir)/include \
	$(ANDROID_HEADERS_CFLAGS) \
	-I$(top_srcdir)/common \
	-DLINKER_PLUGIN_DIR=\"$(libdir)/libhybris/linker\"
if WANT_TRACE
libhybris_common_la_CPPFLAGS += -DDEBUG
endif
if WANT_DEBUG
libhybris_common_la_CPPFLAGS += -ggdb -O0
endif
if WANT_MALI_QUIRKS
libhybris_common_la_CPPFLAGS += -DMALI_QUIRKS
endif
if WANT_UBUNTU_LINKER_OVERRIDES
libhybris_common_la_CPPFLAGS += -DUBUNTU_LINKER_OVERRIDES
endif
if !WANT_ARCH_ARM64
libhybris_common_la_CPPFLAGS += -DWANT_LINKER_JB
endif
if WANT_EXPERIMENTAL
libhybris_common_la_CPPFLAGS += -DWANT_LINKER_MM
endif
libhybris_common_la_LDFLAGS = \
	-ldl \
	-lrt \
	-lpthread \
	-lc \
	$(top_builddir)/properties/libandroid-properties.la \
	-version-info "$(LT_CURRENT)":"$(LT_REVISION)":"$(LT_AGE)"
