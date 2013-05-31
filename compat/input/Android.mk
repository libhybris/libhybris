LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

HYBRIS_PATH := $(LOCAL_PATH)/../../hybris

LOCAL_CFLAGS += -std=gnu++0x

LOCAL_SRC_FILES:= input_compatibility_layer.cpp

LOCAL_MODULE:= libis_compat_layer
LOCAL_MODULE_TAGS := optional

LOCAL_SHARED_LIBRARIES := \
	libinput \
	libcutils \
	libutils \
	libskia \
	libgui \
	libandroidfw

LOCAL_C_INCLUDES := \
	$(HYBRIS_PATH)/include \
	external/skia/include/core \
	frameworks/base/services/input

include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)

HYBRIS_PATH := $(LOCAL_PATH)/../../hybris

LOCAL_CFLAGS += -std=gnu++0x

LOCAL_SRC_FILES:= \
	direct_input_test.cpp

LOCAL_MODULE:= direct_input_test
LOCAL_MODULE_TAGS := optional

LOCAL_C_INCLUDES := \
	$(HYBRIS_PATH)/include \
	bionic \
	bionic/libstdc++/include \
	external/gtest/include \
	external/stlport/stlport \
	external/skia/include/core

LOCAL_SHARED_LIBRARIES := \
	libis_compat_layer \
	libcutils \
	libutils \
	libskia \
	libgui \
	libandroidfw

static_libraries := \
	libgtest \
	libgtest_main

include $(BUILD_EXECUTABLE)
