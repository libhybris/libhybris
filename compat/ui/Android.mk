LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)
include $(LOCAL_PATH)/../Android.common.mk

HYBRIS_PATH := $(LOCAL_PATH)/../../hybris

LOCAL_SRC_FILES:= \
	ui_compatibility_layer.cpp

LOCAL_MODULE:= libui_compat_layer
LOCAL_MODULE_TAGS := optional

LOCAL_C_INCLUDES := \
	$(HYBRIS_PATH)/include \
	frameworks/native/include

LOCAL_SHARED_LIBRARIES := \
	libcutils \
	libutils \
	libbinder \
	libhardware \
	liblog \
	libui

ifeq ($(shell test $(ANDROID_VERSION_MAJOR) -ge 10 && echo true),true)
LOCAL_SHARED_LIBRARIES += \
	libhidlbase
endif

include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
