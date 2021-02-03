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
#include <string>
#include <sstream>

#include <gtest/gtest.h>

#include "linked_list.h"

namespace {

bool alloc_called = false;
bool free_called = false;

class LinkedListTestAllocator {
 public:
  typedef LinkedListEntry<const char> entry_t;

  static entry_t* alloc() {
    alloc_called = true;
    return reinterpret_cast<entry_t*>(::malloc(sizeof(entry_t)));
  }

  static void free(entry_t* p) {
    free_called = true;
    ::free(p);
  }
 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(LinkedListTestAllocator);
};

typedef LinkedList<const char, LinkedListTestAllocator> test_list_t;

std::string test_list_to_string(test_list_t& list) {
  std::stringstream ss;
  list.for_each([&] (const char* c) {
    ss << c;
  });

  return ss.str();
}

};

TEST(linked_list, simple) {
  alloc_called = free_called = false;
  test_list_t list;
  ASSERT_EQ("", test_list_to_string(list));
  ASSERT_TRUE(!alloc_called);
  ASSERT_TRUE(!free_called);
  list.push_front("a");
  ASSERT_TRUE(alloc_called);
  ASSERT_TRUE(!free_called);
  ASSERT_EQ("a", test_list_to_string(list));
  list.push_front("b");
  ASSERT_EQ("ba", test_list_to_string(list));
  list.push_front("c");
  list.push_front("d");
  ASSERT_EQ("dcba", test_list_to_string(list));
  ASSERT_TRUE(alloc_called);
  ASSERT_TRUE(!free_called);
  alloc_called = free_called = false;
  list.remove_if([] (const char* c) {
    return *c == 'c';
  });

  ASSERT_TRUE(!alloc_called);
  ASSERT_TRUE(free_called);

  ASSERT_EQ("dba", test_list_to_string(list));
  alloc_called = free_called = false;
  list.remove_if([] (const char* c) {
    return *c == '2';
  });
  ASSERT_TRUE(!alloc_called);
  ASSERT_TRUE(!free_called);
  ASSERT_EQ("dba", test_list_to_string(list));
  list.clear();
  ASSERT_TRUE(!alloc_called);
  ASSERT_TRUE(free_called);
  ASSERT_EQ("", test_list_to_string(list));
}

TEST(linked_list, push_pop) {
  test_list_t list;
  list.push_front("b");
  list.push_front("a");
  ASSERT_EQ("ab", test_list_to_string(list));
  list.push_back("c");
  ASSERT_EQ("abc", test_list_to_string(list));
  ASSERT_STREQ("a", list.pop_front());
  ASSERT_EQ("bc", test_list_to_string(list));
  ASSERT_STREQ("b", list.pop_front());
  ASSERT_EQ("c", test_list_to_string(list));
  ASSERT_STREQ("c", list.pop_front());
  ASSERT_EQ("", test_list_to_string(list));
  ASSERT_TRUE(list.pop_front() == nullptr);
  list.push_back("r");
  ASSERT_EQ("r", test_list_to_string(list));
  ASSERT_STREQ("r", list.pop_front());
  ASSERT_TRUE(list.pop_front() == nullptr);
}

TEST(linked_list, remove_if_then_pop) {
  test_list_t list;
  list.push_back("a");
  list.push_back("b");
  list.push_back("c");
  list.push_back("d");
  list.remove_if([](const char* c) {
    return *c == 'b' || *c == 'c';
  });

  ASSERT_EQ("ad", test_list_to_string(list));
  ASSERT_STREQ("a", list.pop_front());
  ASSERT_EQ("d", test_list_to_string(list));
  ASSERT_STREQ("d", list.pop_front());
  ASSERT_TRUE(list.pop_front() == nullptr);
}

TEST(linked_list, remove_if_last_then_push_back) {
  test_list_t list;

  list.push_back("a");
  list.push_back("b");
  list.push_back("c");
  list.push_back("d");

  list.remove_if([](const char* c) {
    return *c == 'c' || *c == 'd';
  });

  ASSERT_EQ("ab", test_list_to_string(list));
  list.push_back("d");
  ASSERT_EQ("abd", test_list_to_string(list));
}

TEST(linked_list, copy_to_array) {
  test_list_t list;
  const size_t max_size = 128;
  const char* buf[max_size];
  memset(buf, 0, sizeof(buf));

  ASSERT_EQ(0U, list.copy_to_array(buf, max_size));
  ASSERT_EQ(nullptr, buf[0]);

  list.push_back("a");
  list.push_back("b");
  list.push_back("c");
  list.push_back("d");

  memset(buf, 0, sizeof(buf));
  ASSERT_EQ(2U, list.copy_to_array(buf, 2));
  ASSERT_STREQ("a", buf[0]);
  ASSERT_STREQ("b", buf[1]);
  ASSERT_EQ(nullptr, buf[2]);

  ASSERT_EQ(4U, list.copy_to_array(buf, max_size));
  ASSERT_STREQ("a", buf[0]);
  ASSERT_STREQ("b", buf[1]);
  ASSERT_STREQ("c", buf[2]);
  ASSERT_STREQ("d", buf[3]);
  ASSERT_EQ(nullptr, buf[4]);

  memset(buf, 0, sizeof(buf));
  list.remove_if([](const char* c) {
    return *c != 'c';
  });
  ASSERT_EQ(1U, list.copy_to_array(buf, max_size));
  ASSERT_STREQ("c", buf[0]);
  ASSERT_EQ(nullptr, buf[1]);

  memset(buf, 0, sizeof(buf));

  list.remove_if([](const char* c) {
    return *c == 'c';
  });

  ASSERT_EQ(0U, list.copy_to_array(buf, max_size));
  ASSERT_EQ(nullptr, buf[0]);
}

TEST(linked_list, test_visit) {
  test_list_t list;
  list.push_back("a");
  list.push_back("b");
  list.push_back("c");
  list.push_back("d");

  int visits = 0;
  std::stringstream ss;
  bool result = list.visit([&](const char* c) {
    ++visits;
    ss << c;
    return true;
  });

  ASSERT_TRUE(result);
  ASSERT_EQ(4, visits);
  ASSERT_EQ("abcd", ss.str());

  visits = 0;
  ss.str(std::string());

  result = list.visit([&](const char* c) {
    if (++visits == 3) {
      return false;
    }

    ss << c;
    return true;
  });

  ASSERT_TRUE(!result);
  ASSERT_EQ(3, visits);
  ASSERT_EQ("ab", ss.str());
}

