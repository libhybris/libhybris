LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
    debugger.cpp \
    dlfcn.cpp \
    linker.cpp \
    linker_allocator.cpp \
    linker_sdk_versions.cpp \
    linker_block_allocator.cpp \
    linker_libc_support.c \
    linker_memory.cpp \
    linker_phdr.cpp \
    rt.cpp \

LOCAL_SRC_FILES_arm     := arch/arm/begin.S
LOCAL_SRC_FILES_arm64   := arch/arm64/begin.S
LOCAL_SRC_FILES_x86     := arch/x86/begin.c
LOCAL_SRC_FILES_x86_64  := arch/x86_64/begin.S
LOCAL_SRC_FILES_mips    := arch/mips/begin.S linker_mips.cpp
LOCAL_SRC_FILES_mips64  := arch/mips64/begin.S linker_mips.cpp

# -shared is used to overwrite the -Bstatic and -static
# flags triggered by LOCAL_FORCE_STATIC_EXECUTABLE.
# This dynamic linker is actually a shared object linked with static libraries.
LOCAL_LDFLAGS := \
    -shared \
    -Wl,-Bsymbolic \
    -Wl,--exclude-libs,ALL \

LOCAL_CFLAGS += \
    -fno-stack-protector \
    -Wstrict-overflow=5 \
    -fvisibility=hidden \
    -Wall -Wextra -Wunused -Werror \

LOCAL_CFLAGS_arm += -D__work_around_b_19059885__
LOCAL_CFLAGS_x86 += -D__work_around_b_19059885__

LOCAL_CONLYFLAGS += \
    -std=gnu99 \

LOCAL_CPPFLAGS += \
    -std=gnu++11 \
    -Wold-style-cast \

ifeq ($(TARGET_IS_64_BIT),true)
LOCAL_CPPFLAGS += -DTARGET_IS_64_BIT
endif

# We need to access Bionic private headers in the linker.
LOCAL_CFLAGS += -I$(LOCAL_PATH)/../libc/

# we don't want crtbegin.o (because we have begin.o), so unset it
# just for this module
LOCAL_NO_CRT := true
# TODO: split out the asflags.
LOCAL_ASFLAGS := $(LOCAL_CFLAGS)

LOCAL_ADDITIONAL_DEPENDENCIES := $(LOCAL_PATH)/Android.mk

LOCAL_STATIC_LIBRARIES := libc_nomalloc libziparchive libutils libz liblog

LOCAL_FORCE_STATIC_EXECUTABLE := true

LOCAL_MODULE := linker
LOCAL_MODULE_STEM_32 := linker
LOCAL_MODULE_STEM_64 := linker64
LOCAL_MULTILIB := both

# Leave the symbols in the shared library so that stack unwinders can produce
# meaningful name resolution.
LOCAL_STRIP_MODULE := keep_symbols

# Insert an extra objcopy step to add prefix to symbols. This is needed to prevent gdb
# looking up symbols in the linker by mistake.
#
# Note we are using "=" instead of ":=" to defer the evaluation,
# because LOCAL_2ND_ARCH_VAR_PREFIX or linked_module isn't set properly yet at this point.
LOCAL_POST_LINK_CMD = $(hide) $($(LOCAL_2ND_ARCH_VAR_PREFIX)TARGET_OBJCOPY) \
  --prefix-symbols=__dl_ $(linked_module)

include $(BUILD_EXECUTABLE)

include $(call first-makefiles-under,$(LOCAL_PATH))
