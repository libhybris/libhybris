bionic = $(top_srcdir)/common/mm/bionic
bionic_libc = $(bionic)/libc
bionic_libstdcpp = $(bionic)/libstdc++

if WANT_ARCH_ARM
ARCH_FLAGS = \
	-DHAVE_ARM_TLS_REGISTER \
	-DANDROID_ARM_LINKER
ARCH_NAME = arm
endif

if  WANT_ARCH_X86
ARCH_FLAGS = \
	-DANDROID_X86_LINKER
ARCH_NAME = x86
endif

if WANT_ARCH_X86_64
ARCH_FLAGS = \
	-DANDROID_X86_LINKER
ARCH_NAME = x86_64
endif

AM_CFLAGS = \
	-std=gnu99

AM_CPPFLAGS = \
	-std=gnu++11 \
	-Wold-style-cast \
	-fno-stack-protector \
	-Wstrict-overflow=5

noinst_LTLIBRARIES = \
	libandroid-linker.la
libandroid_linker_la_SOURCES = \
	hybris_compat.cpp \
	dlfcn.cpp \
	linker_allocator.cpp \
	linker_block_allocator.cpp \
	linker.cpp \
	linker_memory.cpp \
	linker_phdr.cpp \
	linker_sdk_versions.cpp \
	rt.cpp
libandroid_linker_la_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/common \
	-I$(bionic_libc)/ \
	-I$(bionic_libc)/include \
	-I$(bionic_libc)/arch-${ARCH_NAME}/include \
	-I$(bionic_libstdcpp)/include \
	-DLINKER_TEXT_BASE=0xB0000100 \
	-DLINKER_AREA_SIZE=0x01000000 \
	-DDEFAULT_HYBRIS_LD_LIBRARY_PATH="\"@DEFAULT_HYBRIS_LD_LIBRARY_PATH@\"" \
	$(ARCH_FLAGS) \
	$(ANDROID_HEADERS_CFLAGS)
libandroid_linker_la_LDFLAGS = \
	-lsupc++

if WANT_DEBUG
libandroid_linker_la_CPPFLAGS += -DLINKER_DEBUG=1
else
libandroid_linker_la_CPPFLAGS += -DLINKER_DEBUG=0
endif
