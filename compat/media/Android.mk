LOCAL_PATH:= $(call my-dir)

ifeq (,$(wildcard frameworks/av/media/libmediaplayerservice/Android.mk))
HYBRIS_MEDIA_32_BIT_ONLY := $(shell cat frameworks/av/media/libmediaplayerservice/Android.mk |grep LOCAL_32_BIT_ONLY |grep -o "true\|false")
endif

ifeq ($(HYBRIS_MEDIA_32_BIT_ONLY),true)
HYBRIS_MEDIA_MULTILIB := 32
endif

include $(CLEAR_VARS)
include $(LOCAL_PATH)/../Android.common.mk

ifeq ($(CAMERA_SERVICE_WANT_UBUNTU_HEADERS),1)
    LOCAL_CPPFLAGS += -DWANT_UBUNTU_CAMERA_HEADERS
endif

LOCAL_SRC_FILES := \
	camera_service.cpp

LOCAL_SHARED_LIBRARIES := \
	libcameraservice \
	libcamera_client \
	libmedialogservice \
	libcutils \
	libmedia \
	libmedia_compat_layer \
	libmediaplayerservice \
	libutils \
	liblog \
	libbinder

LOCAL_C_INCLUDES := \
    frameworks/av/media/libmediaplayerservice \
    frameworks/av/services/medialog \
    frameworks/av/services/camera/libcameraservice

IS_ANDROID_5 := $(shell test $(ANDROID_VERSION_MAJOR) -ge 5 && echo true)

ifeq ($(IS_ANDROID_5),true)
LOCAL_C_INCLUDES += system/media/camera/include

# All devices having Android 5.x also have MediaCodecSource
# available so we don't have to put a switch for this into
# any BoardConfig.mk
BOARD_HAS_MEDIA_CODEC_SOURCE := true
endif

LOCAL_MODULE := camera_service

ifdef TARGET_2ND_ARCH
LOCAL_MULTILIB := both
LOCAL_MODULE_STEM_32 := $(if $(filter false,$(BOARD_UBUNTU_PREFER_32_BIT)),$(LOCAL_MODULE)$(TARGET_2ND_ARCH_MODULE_SUFFIX),$(LOCAL_MODULE))
LOCAL_MODULE_STEM_64 := $(if $(filter false,$(BOARD_UBUNTU_PREFER_32_BIT)),$(LOCAL_MODULE),$(LOCAL_MODULE)_64)
endif

ifeq ($(HYBRIS_MEDIA_32_BIT_ONLY),true)
LOCAL_32_BIT_ONLY := true
LOCAL_MULTILIB := 32
endif

include $(BUILD_EXECUTABLE)

# -------------------------------------------------

include $(CLEAR_VARS)
include $(LOCAL_PATH)/../Android.common.mk

HYBRIS_PATH := $(LOCAL_PATH)/../../hybris

LOCAL_CFLAGS += -std=gnu++0x

ifeq ($(BOARD_HAS_MEDIA_RECORDER_PAUSE),true)
LOCAL_CFLAGS += -DBOARD_HAS_MEDIA_RECORDER_PAUSE
endif
ifeq ($(BOARD_HAS_MEDIA_RECORDER_RESUME),true)
LOCAL_CFLAGS += -DBOARD_HAS_MEDIA_RECORDER_RESUME
endif

LOCAL_SRC_FILES:= \
	media_compatibility_layer.cpp \
	media_codec_layer.cpp \
	media_codec_list.cpp \
	media_format_layer.cpp \
	surface_texture_client_hybris.cpp \
	decoding_service.cpp \
	media_recorder_layer.cpp \
	media_recorder.cpp \
	media_recorder_client.cpp \
	media_recorder_factory.cpp \
	media_recorder_observer.cpp \
	media_buffer_layer.cpp \
	media_message_layer.cpp \
	media_meta_data_layer.cpp

ifeq ($(BOARD_HAS_MEDIA_CODEC_SOURCE),true)
# MediaCodecSource support is only available starting with
# Android 5.x so we have to limit support for it.
LOCAL_SRC_FILES += media_codec_source_layer.cpp
endif

LOCAL_MODULE:= libmedia_compat_layer
LOCAL_MODULE_TAGS := optional

LOCAL_SHARED_LIBRARIES := \
	libcutils \
	libcamera_client \
	libutils \
	libbinder \
	libhardware \
	libui \
	libgui \
	libstagefright \
	libstagefright_foundation \
	libEGL \
	libGLESv2 \
	libmedia \
	libaudioutils \
	libmediaplayerservice

LOCAL_C_INCLUDES := \
	$(HYBRIS_PATH)/include \
	frameworks/base/media/libstagefright/include \
	frameworks/base/include/media/stagefright \
	frameworks/base/include/media \
	frameworks/av/media \
	frameworks/av/media/libstagefright/include \
	frameworks/av/include \
	frameworks/native/include \
	frameworks/native/include/media/hardware \
	system/media/audio_utils/include \
	frameworks/av/services/camera/libcameraservice

IS_ANDROID_5 := $(shell test $(ANDROID_VERSION_MAJOR) -ge 5 && echo true)
ifeq ($(IS_ANDROID_5),true)
LOCAL_C_INCLUDES += frameworks/native/include/media/openmax
endif

ifeq ($(strip $(MTK_CAMERA_BSP_SUPPORT)),yes)
LOCAL_C_INCLUDES += $(TOP)/mediatek/kernel/include/linux/vcodec
LOCAL_SHARED_LIBRARIES += \
	libvcodecdrv

LOCAL_C_INCLUDES+= \
	$(TOP)/$(MTK_PATH_SOURCE)/frameworks-ext/av/media/libmediaplayerservice \
	$(TOP)/$(MTK_PATH_SOURCE)/frameworks-ext/av/include \
	$(TOP)/$(MTK_PATH_SOURCE)/frameworks-ext/av/media/libstagefright/include \
	$(TOP)/$(MTK_PATH_PLATFORM)/frameworks/libmtkplayer \
	$(TOP)/$(MTK_PATH_SOURCE)/frameworks/av/include
endif

ifeq ($(HYBRIS_MEDIA_32_BIT_ONLY),true)
LOCAL_32_BIT_ONLY := true
LOCAL_MULTILIB := 32
endif

include $(BUILD_SHARED_LIBRARY)

# -------------------------------------------------

include $(CLEAR_VARS)
include $(LOCAL_PATH)/../Android.common.mk

LOCAL_SRC_FILES:= \
	direct_media_test.cpp

LOCAL_MODULE:= direct_media_test
LOCAL_MODULE_TAGS := optional

LOCAL_C_INCLUDES := \
	$(HYBRIS_PATH)/include \
	bionic \
	external/libcxx/include \
	external/gtest/include \
	external/skia/include/core \
	frameworks/base/include

LOCAL_SHARED_LIBRARIES := \
	libis_compat_layer \
	libsf_compat_layer \
	libmedia_compat_layer \
	libcutils \
	libutils \
	libbinder \
	libhardware \
	libui \
	libgui \
	libEGL \
	libGLESv2

ifdef TARGET_2ND_ARCH
LOCAL_MULTILIB := both
LOCAL_MODULE_STEM_32 := $(if $(filter false,$(BOARD_UBUNTU_PREFER_32_BIT)),$(LOCAL_MODULE)$(TARGET_2ND_ARCH_MODULE_SUFFIX),$(LOCAL_MODULE))
LOCAL_MODULE_STEM_64 := $(if $(filter false,$(BOARD_UBUNTU_PREFER_32_BIT)),$(LOCAL_MODULE),$(LOCAL_MODULE)_64)
endif

ifeq ($(HYBRIS_MEDIA_32_BIT_ONLY),true)
LOCAL_32_BIT_ONLY := true
LOCAL_MULTILIB := 32
endif

include $(BUILD_EXECUTABLE)
