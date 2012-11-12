lib_LTLIBRARIES = \
	libhybris-common.la

# FIXME this has to be detected at configuration phase
ARCHFLAGS = -DHAVE_ARM_TLS_REGISTER -DANDROID_ARM_LINKER
# ARCHFLAGS = -DANDROID_X86_LINKER

libhybris_common_la_SOURCES = \
	hooks.c \
	properties.c \
	strlcpy.c
libhybris_common_la_SOURCES += \
	gingerbread/dlfcn.c \
	gingerbread/linker.c \
	gingerbread/linker_environ.c \
	gingerbread/linker_format.c \
	gingerbread/rt.c
libhybris_common_la_CFLAGS = \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/common \
	-I$(top_srcdir)/common/gingerbread \
	-DLINKER_DEBUG=1 \
	-DLINKER_TEXT_BASE=0xB0000100 \
	-DLINKER_AREA_SIZE=0x01000000 \
	$(ARCHFLAGS)
if WANT_DEBUG
libhybris_common_la_CFLAGS += -ggdb -O0 -DDEBUG
endif
libhybris_common_la_LDFLAGS = \
	-ldl \
	-pthread \
	-version-info "$(LT_CURRENT)":"$(LT_REVISION)":"$(LT_AGE)"

