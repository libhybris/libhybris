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

#pragma once

#include <link.h>
#include <stddef.h>

#include <string>
#include <unordered_map>

#include <async_safe/log.h>

#define DL_ERR(fmt, x...) \
    do { \
      async_safe_format_buffer(linker_get_error_buffer(), linker_get_error_buffer_size(), fmt, ##x); \
    } while (false)

#define DL_WARN(fmt, x...) \
    do { \
      async_safe_format_log(ANDROID_LOG_WARN, "linker", fmt, ##x); \
      async_safe_format_fd(2, "WARNING: linker: "); \
      async_safe_format_fd(2, fmt, ##x); \
      async_safe_format_fd(2, "\n"); \
    } while (false)

void DL_WARN_documented_change(int api_level, const char* doc_link, const char* fmt, ...);

#define DL_ERR_AND_LOG(fmt, x...) \
  do { \
    DL_ERR(fmt, x); \
    PRINT(fmt, x); \
  } while (false)

constexpr ElfW(Versym) kVersymNotNeeded = 0;
constexpr ElfW(Versym) kVersymGlobal = 1;

// These values are used to call constructors for .init_array && .preinit_array
extern int g_argc;
extern char** g_argv;
extern char** g_envp;

struct soinfo;
struct android_namespace_t;

extern android_namespace_t g_default_namespace;

extern std::unordered_map<uintptr_t, soinfo*> g_soinfo_handles_map;

// Error buffer "variable"
char* linker_get_error_buffer();
size_t linker_get_error_buffer_size();

class DlErrorRestorer {
 public:
  DlErrorRestorer() {
    saved_error_msg_ = linker_get_error_buffer();
  }
  ~DlErrorRestorer() {
    strlcpy(linker_get_error_buffer(), saved_error_msg_.c_str(), linker_get_error_buffer_size());
  }
 private:
  std::string saved_error_msg_;
};
