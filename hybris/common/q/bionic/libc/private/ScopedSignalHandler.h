/*
 * Copyright (C) 2012 The Android Open Source Project
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

class ScopedSignalHandler {
 public:
  ScopedSignalHandler(int signal_number, void (*handler)(int), int sa_flags = 0)
      : signal_number_(signal_number) {
    action_ = { .sa_flags = sa_flags, .sa_handler = handler };
    sigaction64(signal_number_, &action_, &old_action_);
  }

  ScopedSignalHandler(int signal_number, void (*action)(int, siginfo_t*, void*),
                      int sa_flags = SA_SIGINFO)
      : signal_number_(signal_number) {
    action_ = { .sa_flags = sa_flags, .sa_sigaction = action };
    sigaction64(signal_number_, &action_, &old_action_);
  }

  explicit ScopedSignalHandler(int signal_number) : signal_number_(signal_number) {
    sigaction64(signal_number, nullptr, &old_action_);
  }

  ~ScopedSignalHandler() {
    sigaction64(signal_number_, &old_action_, nullptr);
  }

  struct sigaction64 action_;
  struct sigaction64 old_action_;
  const int signal_number_;
};
