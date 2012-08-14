
COMMON_FLAGS=

ifeq ($(ARCH),arm)
	ARCHFLAGS = -DHAVE_ARM_TLS_REGISTER -DANDROID_ARM_LINKER
else
	ARCHFLAGS = -DANDROID_X86_LINKER
endif


COMMON_SOURCES=common/strlcpy.c common/hooks.c common/properties.c

GINGERBREAD_SOURCES=gingerbread/linker.c gingerbread/dlfcn.c gingerbread/rt.c gingerbread/linker_environ.c gingerbread/linker_format.c

all: libhybris_gingerbread.so test_gingerbread

libhybris_gingerbread.so: $(COMMON_SOURCES) $(GINGERBREAD_SOURCES)
	$(CC) -g -shared -o $@ -ldl -pthread -fPIC -Igingerbread -Icommon -DLINKER_DEBUG=1 -DLINKER_TEXT_BASE=0xB0000100 -DLINKER_AREA_SIZE=0x01000000 $(ARCHFLAGS) \
		$(GINGERBREAD_SOURCES) $(COMMON_SOURCES)

test_gingerbread: libhybris_gingerbread.so test.c
	$(CC) -g -o $@ -ldl test.c $<
	
clean:
	rm -rf libhybris_gingerbread.so test_gingerbread
