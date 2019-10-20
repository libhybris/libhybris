# define ANDROID_VERSION MAJOR, MINOR and PATCH

ANDROID_VERSION_MAJOR := $(word 1, $(subst ., , $(PLATFORM_VERSION)))
ANDROID_VERSION_MINOR := $(word 2, $(subst ., , $(PLATFORM_VERSION)))
ANDROID_VERSION_PATCH := $(word 3, $(subst ., , $(PLATFORM_VERSION)))

LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := libhwc2_compat_layer
LOCAL_SRC_FILES := HWC2.cpp ComposerHal.cpp hwc2_compatibility_layer.cpp

LOCAL_C_INCLUDES := ../hybris/include

ifeq ($(shell test $(ANDROID_VERSION_MAJOR) -ge 9 && echo true),true)
LOCAL_C_INCLUDES += \
    hardware/interfaces/graphics/composer/2.1/utils/command-buffer/include
LOCAL_HEADER_LIBRARIES := \
    android.hardware.graphics.composer@2.1-command-buffer
else
LOCAL_STATIC_LIBRARIES := \
    libhwcomposer-command-buffer
endif

LOCAL_SHARED_LIBRARIES := \
    android.frameworks.vr.composer@1.0 \
    android.hardware.graphics.allocator@2.0 \
    android.hardware.graphics.composer@2.1 \
    android.hardware.configstore@1.0 \
    android.hardware.configstore-utils \
    libcutils \
    liblog \
    libfmq \
    libhardware \
    libhidlbase \
    libhidltransport \
    libhwbinder \
    libutils \
    libEGL \
    libGLESv1_CM \
    libGLESv2 \
    libbinder \
    libui \
    libgui \
    libsync \
    libbase

LOCAL_EXPORT_SHARED_LIBRARY_HEADERS := \
    android.hardware.graphics.allocator@2.0 \
    android.hardware.graphics.composer@2.1 \
    libhidlbase \
    libhidltransport \
    libhwbinder

LOCAL_CFLAGS += -Wno-unused-parameter -DGL_GLEXT_PROTOTYPES -UNDEBUG
LOCAL_CFLAGS += \
    -DANDROID_VERSION_MAJOR=$(ANDROID_VERSION_MAJOR) \
    -DANDROID_VERSION_MINOR=$(ANDROID_VERSION_MINOR) \
    -DANDROID_VERSION_PATCH=$(ANDROID_VERSION_PATCH)

include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libhybris-gralloc
LOCAL_SRC_FILES := tests/hybris-gralloc.c \
    GrallocUsageConversion.cpp
LOCAL_C_INCLUDES := $(LOCAL_PATH)/tests
LOCAL_SHARED_LIBRARIES := libcutils
LOCAL_CFLAGS := \
    -DANDROID_VERSION_MAJOR=$(ANDROID_VERSION_MAJOR) \
    -DANDROID_VERSION_MINOR=$(ANDROID_VERSION_MINOR) \
    -DANDROID_VERSION_PATCH=$(ANDROID_VERSION_PATCH) \
    -DANDROID_BUILD=1
LOCAL_CFLAGS += -Wno-unused-parameter -UNDEBUG -DHAS_GRALLOC1_HEADER=1
include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libhwcnativewindow
LOCAL_SRC_FILES := tests/hwcomposer_window.cpp \
    tests/nativewindowbase.cpp
LOCAL_SHARED_LIBRARIES := libsync liblog libnativewindow
LOCAL_CFLAGS := \
    -DANDROID_VERSION_MAJOR=$(ANDROID_VERSION_MAJOR) \
    -DANDROID_VERSION_MINOR=$(ANDROID_VERSION_MINOR) \
    -DANDROID_VERSION_PATCH=$(ANDROID_VERSION_PATCH) \
    -DANDROID_BUILD=1
LOCAL_CFLAGS += -Wno-unused-parameter -UNDEBUG
include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := direct_hwc2_test
LOCAL_SRC_FILES := tests/direct_hwc2_test.cpp
LOCAL_C_INCLUDES := $(LOCAL_PATH)/tests
ifdef TARGET_2ND_ARCH
LOCAL_MULTILIB := both
LOCAL_MODULE_STEM_32 := $(if $(filter false,$(BOARD_UBUNTU_PREFER_32_BIT)),$(LOCAL_MODULE)$(TARGET_2ND_ARCH_MODULE_SUFFIX),$(LOCAL_MODULE))
LOCAL_MODULE_STEM_64 := $(if $(filter false,$(BOARD_UBUNTU_PREFER_32_BIT)),$(LOCAL_MODULE),$(LOCAL_MODULE)_64)
endif

LOCAL_STATIC_LIBRARIES := \
    libhwcnativewindow libhybris-gralloc

LOCAL_SHARED_LIBRARIES := \
    libcutils \
    liblog \
    libhardware \
    libnativewindow \
    libsync \
    libEGL \
    libGLESv2 \
    libhwc2_compat_layer

LOCAL_CFLAGS += -Wno-unused-parameter -DGL_GLEXT_PROTOTYPES -UNDEBUG \
    -DHWC2_USE_CPP11 -DHWC2_INCLUDE_STRINGIFICATION
LOCAL_CFLAGS += \
	-DANDROID_VERSION_MAJOR=$(ANDROID_VERSION_MAJOR) \
	-DANDROID_VERSION_MINOR=$(ANDROID_VERSION_MINOR) \
	-DANDROID_VERSION_PATCH=$(ANDROID_VERSION_PATCH)

include $(BUILD_EXECUTABLE)
