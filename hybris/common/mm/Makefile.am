if WANT_ARCH_ARM
ARCHFLAGS = -DHAVE_ARM_TLS_REGISTER -DANDROID_ARM_LINKER
endif

if  WANT_ARCH_X86
ARCHFLAGS = -DANDROID_X86_LINKER
endif

noinst_LTLIBRARIES = \
	libandroid-linker.la

libandroid_linker_la_SOURCES = \
    linker_dlfcn.cpp \
    linker.cpp \
    linker_sdk_versions.cpp \
    linker_block_allocator.cpp \
    linker_phdr.cpp \
    rt.cpp \
	libc_logging.cpp \
	assert.cpp

libandroid_linker_la_CPPFLAGS = \
	-I$(top_srcdir)/include \
	$(ANDROID_HEADERS_CFLAGS) \
	-I$(top_srcdir)/common \
	-D_GNU_SOURCE \
	-D__work_around_b_19059885__ \
	-DDEFAULT_HYBRIS_LD_LIBRARY_PATH="\"@DEFAULT_HYBRIS_LD_LIBRARY_PATH@\"" \
	$(ARCHFLAGS) \
	-std=c++0x \
	-Wno-unused-result

libandroid_linker_la_CFLAGS = \
	-I$(top_srcdir)/include \
	$(ANDROID_HEADERS_CFLAGS) \
	-I$(top_srcdir)/common \
	-D_GNU_SOURCE \
	-D__work_around_b_19059885__ \
	-DDEFAULT_HYBRIS_LD_LIBRARY_PATH="\"@DEFAULT_HYBRIS_LD_LIBRARY_PATH@\"" \
	$(ARCHFLAGS)

if WANT_DEBUG
libandroid_linker_la_CPPFLAGS += -DLINKER_DEBUG=1 -O0 -ggdb
libandroid_linker_la_CFLAGS += -DLINKER_DEBUG=1 -O0 -ggdb
else
libandroid_linker_la_CPPFLAGS += -DLINKER_DEBUG=0
libandroid_linker_la_CFLAGS += -DLINKER_DEBUG=0
endif
