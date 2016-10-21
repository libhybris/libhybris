#
# Copyright (C) 2012 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := linker-unit-tests
LOCAL_MODULE_STEM_32 := $(LOCAL_MODULE)32
LOCAL_MODULE_STEM_64 := $(LOCAL_MODULE)64

LOCAL_ADDITIONAL_DEPENDENCIES := $(LOCAL_PATH)/Android.mk

LOCAL_CFLAGS += -g -Wall -Wextra -Wunused -Werror -std=gnu++11
LOCAL_C_INCLUDES := $(LOCAL_PATH)/../../libc/

LOCAL_SRC_FILES := \
  linked_list_test.cpp \
  linker_block_allocator_test.cpp \
  ../linker_block_allocator.cpp \
  linker_memory_allocator_test.cpp \
  ../linker_allocator.cpp

# for __libc_fatal
LOCAL_SRC_FILES += ../../libc/bionic/libc_logging.cpp

include $(BUILD_NATIVE_TEST)
