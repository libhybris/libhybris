/*
 * Copyright (C) 2016 The Android Open Source Project
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

#include "linker_sleb128.h"

TEST(linker_sleb128, smoke) {
  std::vector<uint8_t> encoding;
  // 624485
  encoding.push_back(0xe5);
  encoding.push_back(0x8e);
  encoding.push_back(0x26);
  // 0
  encoding.push_back(0x00);
  // 1
  encoding.push_back(0x01);
  // 63
  encoding.push_back(0x3f);
  // 64
  encoding.push_back(0xc0);
  encoding.push_back(0x00);
  // -1
  encoding.push_back(0x7f);
  // -624485
  encoding.push_back(0x9b);
  encoding.push_back(0xf1);
  encoding.push_back(0x59);
  // 2147483647
  encoding.push_back(0xff);
  encoding.push_back(0xff);
  encoding.push_back(0xff);
  encoding.push_back(0xff);
  encoding.push_back(0x07);
  // -2147483648
  encoding.push_back(0x80);
  encoding.push_back(0x80);
  encoding.push_back(0x80);
  encoding.push_back(0x80);
  encoding.push_back(0x78);
#if defined(__LP64__)
  // 9223372036854775807
  encoding.push_back(0xff);
  encoding.push_back(0xff);
  encoding.push_back(0xff);
  encoding.push_back(0xff);
  encoding.push_back(0xff);
  encoding.push_back(0xff);
  encoding.push_back(0xff);
  encoding.push_back(0xff);
  encoding.push_back(0xff);
  encoding.push_back(0x00);
  // -9223372036854775808
  encoding.push_back(0x80);
  encoding.push_back(0x80);
  encoding.push_back(0x80);
  encoding.push_back(0x80);
  encoding.push_back(0x80);
  encoding.push_back(0x80);
  encoding.push_back(0x80);
  encoding.push_back(0x80);
  encoding.push_back(0x80);
  encoding.push_back(0x7f);
#endif
  sleb128_decoder decoder(&encoding[0], encoding.size());

  EXPECT_EQ(624485U, decoder.pop_front());

  EXPECT_EQ(0U, decoder.pop_front());
  EXPECT_EQ(1U, decoder.pop_front());
  EXPECT_EQ(63U, decoder.pop_front());
  EXPECT_EQ(64U, decoder.pop_front());
  EXPECT_EQ(static_cast<size_t>(-1), decoder.pop_front());
  EXPECT_EQ(static_cast<size_t>(-624485), decoder.pop_front());
  EXPECT_EQ(2147483647U, decoder.pop_front());
  EXPECT_EQ(static_cast<size_t>(-2147483648), decoder.pop_front());
#if defined(__LP64__)
  EXPECT_EQ(9223372036854775807ULL, decoder.pop_front());
  EXPECT_EQ(static_cast<uint64_t>(-9223372036854775807LL - 1), decoder.pop_front());
#endif
}
