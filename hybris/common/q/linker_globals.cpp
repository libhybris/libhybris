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

#include "linker.h"
#include "linker_globals.h"
#include "linker_namespaces.h"
#include "linker_utils.h"
#include <stdarg.h>
//#include "android-base/stringprintf.h"

int g_argc = 0;
char** g_argv = nullptr;
char** g_envp = nullptr;

std::unordered_map<uintptr_t, soinfo*> g_soinfo_handles_map;

static char __linker_dl_err_buf[768];

char* linker_get_error_buffer() {
  return &__linker_dl_err_buf[0];
}

size_t linker_get_error_buffer_size() {
  return sizeof(__linker_dl_err_buf);
}

void DL_WARN_documented_change(int api_level, const char* doc_link, const char* fmt, ...) {
  std::string result{"Warning: "};

  va_list ap;
  va_start(ap, fmt);
  stringAppendV(&result, fmt, ap);
  va_end(ap);

  stringAppendF(&result,
                               " and will not work when the app moves to API level %d or later "
                               "(https://android.googlesource.com/platform/bionic/+/master/%s) "
                               "(allowing for now because this app's target API level is still %d)",
                               api_level, doc_link, get_application_target_sdk_version());
  DL_WARN("%s", result.c_str());
}
