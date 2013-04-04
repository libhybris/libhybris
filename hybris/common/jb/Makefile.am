if WANT_ARCH_ARM
ARCHFLAGS = -DHAVE_ARM_TLS_REGISTER -DANDROID_ARM_LINKER
endif

if  WANT_ARCH_X86
ARCHFLAGS = -DANDROID_X86_LINKER
endif

noinst_LTLIBRARIES = \
	libandroid-linker.la
libandroid_linker_la_SOURCES = \
	dlfcn.c \
	linker.c \
	linker_environ.c \
	linker_format.c \
	rt.c
libandroid_linker_la_CFLAGS = \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/common \
	-DLINKER_DEBUG=1 \
	-DLINKER_TEXT_BASE=0xB0000100 \
	-DLINKER_AREA_SIZE=0x01000000 \
	$(ARCHFLAGS)
