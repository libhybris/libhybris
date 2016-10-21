/*
 * Copyright (C) 2013 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

#include <gtest/gtest.h>

#include "../linker_allocator.h"

#include <unistd.h>

namespace {

/*
 * this one has size below allocator cap which is 2*sizeof(void*)
 */
struct test_struct_small {
  char dummy_str[5];
};

struct test_struct_large {
  char dummy_str[1009];
};

struct test_struct_huge {
  char dummy_str[73939];
};

struct test_struct_512 {
  char dummy_str[503];
};

};

static size_t kPageSize = sysconf(_SC_PAGE_SIZE);

TEST(linker_memory, test_alloc_0) {
  LinkerMemoryAllocator allocator;
  void* ptr = allocator.alloc(0);
  ASSERT_TRUE(ptr != nullptr);
  free(ptr);
}

TEST(linker_memory, test_free_nullptr) {
  LinkerMemoryAllocator allocator;
  allocator.free(nullptr);
}

TEST(linker_memory, test_realloc) {
  LinkerMemoryAllocator allocator;
  uint32_t* array = reinterpret_cast<uint32_t*>(allocator.alloc(512));
  const size_t array_size = 512 / sizeof(uint32_t);

  uint32_t model[1000];

  model[0] = 1;
  model[1] = 1;

  for (size_t i = 2; i < 1000; ++i) {
    model[i] = model[i - 1] + model[i - 2];
  }

  memcpy(array, model, array_size);

  uint32_t* reallocated_ptr = reinterpret_cast<uint32_t*>(allocator.realloc(array, 1024));

  ASSERT_TRUE(reallocated_ptr != nullptr);
  ASSERT_TRUE(reallocated_ptr != array);

  ASSERT_TRUE(memcmp(reallocated_ptr, model, array_size) == 0);

  array = reallocated_ptr;

  memcpy(array, model, 2*array_size);

  reallocated_ptr = reinterpret_cast<uint32_t*>(allocator.realloc(array, 62));

  ASSERT_TRUE(reallocated_ptr == array);

  reallocated_ptr = reinterpret_cast<uint32_t*>(allocator.realloc(array, 4000));

  ASSERT_TRUE(reallocated_ptr != nullptr);
  ASSERT_TRUE(reallocated_ptr != array);

  ASSERT_TRUE(memcmp(reallocated_ptr, model, array_size * 2) == 0);

  array = reallocated_ptr;

  memcpy(array, model, 4000);

  reallocated_ptr = reinterpret_cast<uint32_t*>(allocator.realloc(array, 64000));

  ASSERT_TRUE(reallocated_ptr != nullptr);
  ASSERT_TRUE(reallocated_ptr != array);

  ASSERT_TRUE(memcmp(reallocated_ptr, model, 4000) == 0);

  ASSERT_EQ(nullptr, realloc(reallocated_ptr, 0));
}

TEST(linker_memory, test_small_smoke) {
  LinkerMemoryAllocator allocator;

  uint8_t zeros[16];
  memset(zeros, 0, sizeof(zeros));

  test_struct_small* ptr1 =
      reinterpret_cast<test_struct_small*>(allocator.alloc(sizeof(test_struct_small)));
  test_struct_small* ptr2 =
      reinterpret_cast<test_struct_small*>(allocator.alloc(sizeof(test_struct_small)));

  ASSERT_TRUE(ptr1 != nullptr);
  ASSERT_TRUE(ptr2 != nullptr);
  ASSERT_EQ(reinterpret_cast<uintptr_t>(ptr1)+16, reinterpret_cast<uintptr_t>(ptr2));
  ASSERT_TRUE(memcmp(ptr1, zeros, 16) == 0);

  allocator.free(ptr1);
  allocator.free(ptr2);
}

TEST(linker_memory, test_huge_smoke) {
  LinkerMemoryAllocator allocator;

  // this should trigger proxy-to-mmap
  test_struct_huge* ptr1 =
      reinterpret_cast<test_struct_huge*>(allocator.alloc(sizeof(test_struct_huge)));
  test_struct_huge* ptr2 =
      reinterpret_cast<test_struct_huge*>(allocator.alloc(sizeof(test_struct_huge)));

  ASSERT_TRUE(ptr1 != nullptr);
  ASSERT_TRUE(ptr2 != nullptr);

  ASSERT_TRUE(
      reinterpret_cast<uintptr_t>(ptr1)/kPageSize != reinterpret_cast<uintptr_t>(ptr2)/kPageSize);
  allocator.free(ptr2);
  allocator.free(ptr1);
}

TEST(linker_memory, test_large) {
  LinkerMemoryAllocator allocator;

  test_struct_large* ptr1 =
      reinterpret_cast<test_struct_large*>(allocator.alloc(sizeof(test_struct_large)));
  test_struct_large* ptr2 =
      reinterpret_cast<test_struct_large*>(allocator.alloc(1024));

  ASSERT_TRUE(ptr1 != nullptr);
  ASSERT_TRUE(ptr2 != nullptr);

  ASSERT_EQ(reinterpret_cast<uintptr_t>(ptr1) + 1024, reinterpret_cast<uintptr_t>(ptr2));

  // let's allocate until we reach the next page.
  size_t n = kPageSize / sizeof(test_struct_large) + 1 - 2;
  test_struct_large* objects[n];

  for (size_t i = 0; i < n; ++i) {
    test_struct_large* obj_ptr =
        reinterpret_cast<test_struct_large*>(allocator.alloc(sizeof(test_struct_large)));
    ASSERT_TRUE(obj_ptr != nullptr);
    objects[i] = obj_ptr;
  }

  test_struct_large* ptr_to_free =
      reinterpret_cast<test_struct_large*>(allocator.alloc(sizeof(test_struct_large)));

  ASSERT_TRUE(ptr_to_free != nullptr);

  allocator.free(ptr1);

  for (size_t i=0; i<n; ++i) {
    allocator.free(objects[i]);
  }

  allocator.free(ptr2);
  allocator.free(ptr_to_free);
}


