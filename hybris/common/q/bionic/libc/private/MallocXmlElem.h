/*
 * Copyright (C) 2014 The Android Open Source Project
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

#include <stdarg.h>
#include <stdio.h>

#include <private/bionic_macros.h>

class MallocXmlElem {
 public:
  // Name must be valid throughout lifetime of the object.
  explicit MallocXmlElem(FILE* fp, const char* name,
                         const char* attr_fmt = nullptr, ...) : fp_(fp), name_(name) {
    fprintf(fp, "<%s", name_);
    if (attr_fmt != nullptr) {
      va_list args;
      va_start(args, attr_fmt);
      fputc(' ', fp_);
      vfprintf(fp_, attr_fmt, args);
      va_end(args);
    }
    fputc('>', fp_);
  }

  ~MallocXmlElem() noexcept {
    fprintf(fp_, "</%s>", name_);
  }

  void Contents(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vfprintf(fp_, fmt, args);
    va_end(args);
  }

private:
  FILE* fp_;
  const char* name_;

  BIONIC_DISALLOW_IMPLICIT_CONSTRUCTORS(MallocXmlElem);
};
