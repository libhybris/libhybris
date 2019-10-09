/*
 * Copyright (C) 2015 The Android Open Source Project
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

#pragma once

#include <errno.h>
#include <stdlib.h>
#include <sys/cdefs.h>
#include <sys/mman.h>
#include <sys/prctl.h>
#include <stddef.h>
#include <unistd.h>

const uint32_t kSmallObjectMaxSizeLog2 = 10;
const uint32_t kSmallObjectMinSizeLog2 = 4;
const uint32_t kSmallObjectAllocatorsCount = kSmallObjectMaxSizeLog2 - kSmallObjectMinSizeLog2 + 1;

class BionicSmallObjectAllocator;

// This structure is placed at the beginning of each addressable page
// and has all information we need to find the corresponding memory allocator.
struct page_info {
  char signature[4];
  uint32_t type;
  union {
    // we use allocated_size for large objects allocator
    size_t allocated_size;
    // and allocator_addr for small ones.
    BionicSmallObjectAllocator* allocator_addr;
  };
};

struct small_object_block_record {
  small_object_block_record* next;
  size_t free_blocks_cnt;
};

// This structure is placed at the beginning of each page managed by
// BionicSmallObjectAllocator.  Note that a page_info struct is expected at the
// beginning of each page as well, and therefore this structure contains a
// page_info as its *first* field.
struct small_object_page_info {
  page_info info;  // Must be the first field.

  // Doubly linked list for traversing all pages allocated by a
  // BionicSmallObjectAllocator.
  small_object_page_info* next_page;
  small_object_page_info* prev_page;

  // Linked list containing all free blocks in this page.
  small_object_block_record* free_block_list;

  // Free blocks counter.
  size_t free_blocks_cnt;
};

class BionicSmallObjectAllocator {
 public:
  BionicSmallObjectAllocator(uint32_t type, size_t block_size);
  void* alloc();
  void free(void* ptr);

  size_t get_block_size() const { return block_size_; }
 private:
  void alloc_page();
  void free_page(small_object_page_info* page);
  void add_to_page_list(small_object_page_info* page);
  void remove_from_page_list(small_object_page_info* page);

  const uint32_t type_;
  const size_t block_size_;
  const size_t blocks_per_page_;

  size_t free_pages_cnt_;

  small_object_page_info* page_list_;
};

class BionicAllocator {
 public:
  constexpr BionicAllocator() : allocators_(nullptr), allocators_buf_() {}
  void* alloc(size_t size);
  void* memalign(size_t align, size_t size);

  // Note that this implementation of realloc never shrinks allocation
  void* realloc(void* ptr, size_t size);
  void free(void* ptr);
 private:
  void* alloc_mmap(size_t align, size_t size);
  inline void* alloc_impl(size_t align, size_t size);
  inline page_info* get_page_info_unchecked(void* ptr);
  inline page_info* get_page_info(void* ptr);
  BionicSmallObjectAllocator* get_small_object_allocator(uint32_t type);
  void initialize_allocators();

  BionicSmallObjectAllocator* allocators_;
  uint8_t allocators_buf_[sizeof(BionicSmallObjectAllocator)*kSmallObjectAllocatorsCount];
};
