lib_LTLIBRARIES = \
	libhybris-common.la

if WANT_ARCH_ARM
ARCHFLAGS = -DHAVE_ARM_TLS_REGISTER -DANDROID_ARM_LINKER
endif

if  WANT_ARCH_X86
ARCHFLAGS = -DANDROID_X86_LINKER
endif

if  WANT_GB
LNKR = gingerbread
endif

if  WANT_ICS
LNKR = ics
endif

if  WANT_JB
LNKR = jb
endif




libhybris_common_la_SOURCES = \
	hooks.c \
	properties.c \
	strlcpy.c
libhybris_common_la_SOURCES += \
	$(LNKR)/dlfcn.c \
	$(LNKR)/linker.c \
	$(LNKR)/linker_environ.c \
	$(LNKR)/linker_format.c \
	$(LNKR)/rt.c
libhybris_common_la_CFLAGS = \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/common \
	-I$(top_srcdir)/common/$(LNKR) \
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

