LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
	arch/$(TARGET_ARCH)/begin.S \
	linker.c \
	linker_environ.c \
	linker_format.c \
	rt.c \
	dlfcn.c \
	debugger.c

LOCAL_LDFLAGS := -shared

LOCAL_CFLAGS += -fno-stack-protector \
        -Wstrict-overflow=5 \
        -fvisibility=hidden

# Set LINKER_DEBUG to either 1 or 0
#
LOCAL_CFLAGS += -DLINKER_DEBUG=0

# we need to access the Bionic private header <bionic_tls.h>
# in the linker; duplicate the HAVE_ARM_TLS_REGISTER definition
# from the libc build
ifeq ($(TARGET_ARCH)-$(ARCH_ARM_HAVE_TLS_REGISTER),arm-true)
    LOCAL_CFLAGS += -DHAVE_ARM_TLS_REGISTER
endif
LOCAL_CFLAGS += -I$(LOCAL_PATH)/../libc/private

ifeq ($(TARGET_ARCH),arm)
LOCAL_CFLAGS += -DANDROID_ARM_LINKER
else
  ifeq ($(TARGET_ARCH),x86)
    LOCAL_CFLAGS += -DANDROID_X86_LINKER
  endif
endif

LOCAL_MODULE:= linker

LOCAL_STATIC_LIBRARIES := libc_nomalloc

#LOCAL_FORCE_STATIC_EXECUTABLE := true # not necessary when not including BUILD_EXECUTABLE

#
# include $(BUILD_EXECUTABLE)
#
# Instead of including $(BUILD_EXECUTABLE), we execute the steps to create an executable by
# hand, as we want to insert an extra step that is not supported by the build system, and
# is probably specific the linker only, so there's no need to modify the build system for
# the purpose.

LOCAL_MODULE_CLASS := EXECUTABLES
LOCAL_MODULE_SUFFIX := $(TARGET_EXECUTABLE_SUFFIX)

# we don't want crtbegin.o (because we have begin.o), so unset it
# just for this module
LOCAL_NO_CRT := true

include $(BUILD_SYSTEM)/dynamic_binary.mk

$(linked_module): $(TARGET_CRTBEGIN_STATIC_O) $(all_objects) $(all_libraries) $(TARGET_CRTEND_O)
	$(transform-o-to-static-executable)
	@echo "target PrefixSymbols: $(PRIVATE_MODULE) ($@)"
	$(hide) $(TARGET_OBJCOPY) --prefix-symbols=__dl_ $@

#
# end of BUILD_EXECUTABLE hack
#
