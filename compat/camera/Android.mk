LOCAL_PATH:= $(call my-dir)

ifneq (,$(wildcard frameworks/av/media/mediaserver/Android.mk))
HYBRIS_MEDIA_32_BIT_ONLY := $(shell cat frameworks/av/media/mediaserver/Android.mk | grep LOCAL_32_BIT_ONLY | grep -o "true\|false")
else
ifneq (,$(wildcard frameworks/av/media/mediaserver/Android.bp))
HYBRIS_MEDIA_32_BIT_ONLY := $(shell cat frameworks/av/media/mediaserver/Android.bp | grep compile_multilib | grep -wo "32" | sed "s/32/true/")
endif
endif

ifeq ($(HYBRIS_MEDIA_32_BIT_ONLY),)
HYBRIS_MEDIA_32_BIT_ONLY := $(shell cat frameworks/av/media/libmediaplayerservice/Android.bp | grep compile_multilib | grep -o "32" | sed "s/32/true/")
endif

ifeq ($(HYBRIS_MEDIA_32_BIT_ONLY),true)
HYBRIS_MEDIA_MULTILIB := 32
endif

include $(CLEAR_VARS)
include $(LOCAL_PATH)/../Android.common.mk

HYBRIS_PATH := $(LOCAL_PATH)/../../hybris

IS_ANDROID_8 := $(shell test $(ANDROID_VERSION_MAJOR) -ge 8 && echo true)

LOCAL_SRC_FILES := camera_compatibility_layer.cpp

LOCAL_MODULE := libcamera_compat_layer
LOCAL_MODULE_TAGS := optional

LOCAL_C_INCLUDES := \
	$(HYBRIS_PATH)/include

ifeq ($(IS_ANDROID_8),true)
LOCAL_CFLAGS += \
	-Wno-unused-parameter \
	-Wno-unused-variable
endif

LOCAL_SHARED_LIBRARIES := \
	libcutils \
	libcamera_client \
	liblog \
	libutils \
	libbinder \
	libhardware \
	libui \
	libgui

ifeq ($(HYBRIS_MEDIA_32_BIT_ONLY),true)
LOCAL_32_BIT_ONLY := true
LOCAL_MULTILIB := 32
endif

include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)

HYBRIS_PATH := $(LOCAL_PATH)/../../hybris

LOCAL_SRC_FILES := direct_camera_test.cpp

LOCAL_MODULE := direct_camera_test
LOCAL_MODULE_TAGS := optional
ifdef TARGET_2ND_ARCH
LOCAL_MULTILIB := both
LOCAL_MODULE_STEM_32 := $(if $(filter false,$(BOARD_UBUNTU_PREFER_32_BIT)),$(LOCAL_MODULE)$(TARGET_2ND_ARCH_MODULE_SUFFIX),$(LOCAL_MODULE))
LOCAL_MODULE_STEM_64 := $(if $(filter false,$(BOARD_UBUNTU_PREFER_32_BIT)),$(LOCAL_MODULE),$(LOCAL_MODULE)_64)
endif

LOCAL_C_INCLUDES := \
	$(HYBRIS_PATH)/include \
	bionic \
	external/libcxx/include \
	external/gtest/include \
	external/skia/include/core \

LOCAL_SHARED_LIBRARIES := \
	libis_compat_layer \
	libsf_compat_layer \
	libcamera_compat_layer \
	libmedia_compat_layer \
	libcutils \
	libcamera_client \
	libutils \
	libbinder \
	libhardware \
	libui \
	libgui \
	libEGL \
	libGLESv2

ifeq ($(HYBRIS_MEDIA_32_BIT_ONLY),true)
LOCAL_32_BIT_ONLY := true
LOCAL_MULTILIB := 32
endif

include $(BUILD_EXECUTABLE)
