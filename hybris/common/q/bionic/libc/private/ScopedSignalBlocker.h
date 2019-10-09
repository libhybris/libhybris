/*
 * Copyright (C) 2017 The Android Open Source Project
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

#pragma once

#include <signal.h>

#include "bionic_macros.h"

class ScopedSignalBlocker {
 public:
  // Block all signals.
  explicit ScopedSignalBlocker() {
    sigset64_t set;
    sigfillset64(&set);
    sigprocmask64(SIG_BLOCK, &set, &old_set_);
  }

  // Block just the specified signal.
  explicit ScopedSignalBlocker(int signal) {
    sigset64_t set = {};
    sigaddset64(&set, signal);
    sigprocmask64(SIG_BLOCK, &set, &old_set_);
  }

  ~ScopedSignalBlocker() {
    reset();
  }

  void reset() {
    sigprocmask64(SIG_SETMASK, &old_set_, nullptr);
  }

  sigset64_t old_set_;

  BIONIC_DISALLOW_COPY_AND_ASSIGN(ScopedSignalBlocker);
};
