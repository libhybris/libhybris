/*
 * Copyright (C) 2013 The Android Open Source Project
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

#include <gtest/gtest.h>

#include "linker_block_allocator.h"

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
  ASSERT_EQ(0U, reinterpret_cast<uintptr_t>(ptr1) % 16);
  test_struct_nominal* ptr2 = allocator.alloc();
  ASSERT_EQ(0U, reinterpret_cast<uintptr_t>(ptr2) % 16);
  ASSERT_TRUE(ptr2 != nullptr);
  // they should be next to each other.
  ASSERT_EQ(reinterpret_cast<uint8_t*>(ptr1)+16, reinterpret_cast<uint8_t*>(ptr2));

  ptr1->value = 42;

  allocator.free(ptr1);
  allocator.free(ptr2);
}

TEST(linker_allocator, test_small) {
  LinkerTypeAllocator<test_struct_small> allocator;

  char* ptr1 = reinterpret_cast<char*>(allocator.alloc());
  char* ptr2 = reinterpret_cast<char*>(allocator.alloc());

  ASSERT_TRUE(ptr1 != nullptr);
  ASSERT_EQ(0U, reinterpret_cast<uintptr_t>(ptr1) % 16);
  ASSERT_TRUE(ptr2 != nullptr);
  ASSERT_EQ(0U, reinterpret_cast<uintptr_t>(ptr2) % 16);
  ASSERT_EQ(ptr1+16, ptr2); // aligned to 16
}

TEST(linker_allocator, test_larger) {
  LinkerTypeAllocator<test_struct_larger> allocator;

  test_struct_larger* ptr1 = allocator.alloc();
  test_struct_larger* ptr2 = allocator.alloc();

  ASSERT_TRUE(ptr1 != nullptr);
  ASSERT_EQ(0U, reinterpret_cast<uintptr_t>(ptr1) % 16);
  ASSERT_TRUE(ptr2 != nullptr);
  ASSERT_EQ(0U, reinterpret_cast<uintptr_t>(ptr2) % 16);

  ASSERT_EQ(reinterpret_cast<uint8_t*>(ptr1) + 1024, reinterpret_cast<uint8_t*>(ptr2));

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
