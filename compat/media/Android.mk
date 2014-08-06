LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	camera_service.cpp

LOCAL_SHARED_LIBRARIES := \
	libcameraservice \
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

LOCAL_MODULE := camera_service

include $(BUILD_EXECUTABLE)

# -------------------------------------------------

include $(CLEAR_VARS)
include $(LOCAL_PATH)/../Android.common.mk

HYBRIS_PATH := $(LOCAL_PATH)/../../hybris

LOCAL_CFLAGS += -std=gnu++0x

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
	media_recorder_factory.cpp

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
	frameworks/av/include \
	frameworks/native/include \
	system/media/audio_utils/include \
	frameworks/av/services/camera/libcameraservice

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
	bionic/libstdc++/include \
	external/gtest/include \
	external/stlport/stlport \
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

include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
include $(LOCAL_PATH)/../Android.common.mk

LOCAL_CFLAGS += -Wno-multichar -D SIMPLE_PLAYER -std=gnu++0x

LOCAL_SRC_FILES:= \
	media_codec_layer.cpp \
	media_codec_list.cpp \
	media_format_layer.cpp \
	codec.cpp \
	SimplePlayer.cpp

LOCAL_SHARED_LIBRARIES := \
	libstagefright \
	libstagefright_foundation \
	liblog \
	libutils \
	libbinder \
	libmedia \
	libgui \
	libcutils \
	libui

LOCAL_C_INCLUDES:= \
	$(HYBRIS_PATH)/include \
	frameworks/av/media/libstagefright \
	frameworks/native/include/media/openmax \
	frameworks/base/media/libstagefright/include \
	frameworks/base/include/media/stagefright \
	frameworks/base/include/media

LOCAL_MODULE:= codec
LOCAL_MODULE_TAGS := optional

include $(BUILD_EXECUTABLE)
