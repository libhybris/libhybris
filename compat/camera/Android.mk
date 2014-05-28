LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)
include $(LOCAL_PATH)/../Android.common.mk

HYBRIS_PATH := $(LOCAL_PATH)/../../hybris

LOCAL_SRC_FILES := camera_compatibility_layer.cpp

LOCAL_MODULE := libcamera_compat_layer
LOCAL_MODULE_TAGS := optional

LOCAL_C_INCLUDES := \
	$(HYBRIS_PATH)/include

LOCAL_SHARED_LIBRARIES := \
	libcutils \
	libcamera_client \
	libutils \
	libbinder \
	libhardware \
	libui \
	libgui

include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)

HYBRIS_PATH := $(LOCAL_PATH)/../../hybris

LOCAL_SRC_FILES := direct_camera_test.cpp

LOCAL_MODULE := direct_camera_test
LOCAL_MODULE_TAGS := optional

LOCAL_C_INCLUDES := \
	$(HYBRIS_PATH)/include \
	bionic \
	bionic/libstdc++/include \
	external/gtest/include \
	external/stlport/stlport \
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

include $(BUILD_EXECUTABLE)
