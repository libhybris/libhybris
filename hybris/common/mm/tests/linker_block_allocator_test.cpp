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

#include "../linker_block_allocator.h"

#include <unistd.h>

namespace {

struct test_struct_nominal {
  void* pointer;
  ssize_t value;
};

/*
 * this one has size below allocator cap which is 2*sizeof(void*)
 */
struct test_struct_small {
  char dummy_str[5];
};

/*
 * 1009 byte struct (1009 is prime)
 */
struct test_struct_larger {
  char dummy_str[1009];
};

static size_t kPageSize = sysconf(_SC_PAGE_SIZE);
};

TEST(linker_allocator, test_nominal) {
  LinkerTypeAllocator<test_struct_nominal> allocator;

  test_struct_nominal* ptr1 = allocator.alloc();
  ASSERT_TRUE(ptr1 != nullptr);
  test_struct_nominal* ptr2 = allocator.alloc();
  ASSERT_TRUE(ptr2 != nullptr);
  // they should be next to each other.
  ASSERT_EQ(ptr1+1, ptr2);

  ptr1->value = 42;

  allocator.free(ptr1);
  allocator.free(ptr2);
}

TEST(linker_allocator, test_small) {
  LinkerTypeAllocator<test_struct_small> allocator;

  char* ptr1 = reinterpret_cast<char*>(allocator.alloc());
  char* ptr2 = reinterpret_cast<char*>(allocator.alloc());

  ASSERT_TRUE(ptr1 != nullptr);
  ASSERT_TRUE(ptr2 != nullptr);
  ASSERT_EQ(ptr1+2*sizeof(void*), ptr2);
}

TEST(linker_allocator, test_larger) {
  LinkerTypeAllocator<test_struct_larger> allocator;

  test_struct_larger* ptr1 = allocator.alloc();
  test_struct_larger* ptr2 = allocator.alloc();

  ASSERT_TRUE(ptr1 != nullptr);
  ASSERT_TRUE(ptr2 != nullptr);

  ASSERT_EQ(ptr1+1, ptr2);

  // lets allocate until we reach next page.
  size_t n = kPageSize/sizeof(test_struct_larger) + 1 - 2;

  for (size_t i=0; i<n; ++i) {
    ASSERT_TRUE(allocator.alloc() != nullptr);
  }

  test_struct_larger* ptr_to_free = allocator.alloc();
  ASSERT_TRUE(ptr_to_free != nullptr);
  allocator.free(ptr1);
}

static void protect_all() {
  LinkerTypeAllocator<test_struct_larger> allocator;

  // number of allocs to reach the end of first page
  size_t n = kPageSize/sizeof(test_struct_larger) - 1;
  test_struct_larger* page1_ptr = allocator.alloc();

  for (size_t i=0; i<n; ++i) {
    allocator.alloc();
  }

  test_struct_larger* page2_ptr = allocator.alloc();
  allocator.protect_all(PROT_READ);
  allocator.protect_all(PROT_READ | PROT_WRITE);
  // check access
  page2_ptr->dummy_str[23] = 27;
  page1_ptr->dummy_str[13] = 11;

  allocator.protect_all(PROT_READ);
  fprintf(stderr, "trying to access protected page");

  // this should result in segmentation fault
  page1_ptr->dummy_str[11] = 7;
}

TEST(linker_allocator, test_protect) {
  testing::FLAGS_gtest_death_test_style = "threadsafe";
  ASSERT_EXIT(protect_all(), testing::KilledBySignal(SIGSEGV), "trying to access protected page");
}

