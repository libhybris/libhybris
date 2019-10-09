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

#ifndef _PRIVATE_ICU_H
#define _PRIVATE_ICU_H

#include <stdint.h>
#include <wchar.h>

typedef int8_t UBool;
#define FALSE 0
#define TRUE 1

typedef int32_t UChar32;

enum UProperty {
  UCHAR_ALPHABETIC = 0,
  UCHAR_DEFAULT_IGNORABLE_CODE_POINT = 5,
  UCHAR_LOWERCASE = 22,
  UCHAR_POSIX_ALNUM = 44,
  UCHAR_POSIX_BLANK = 45,
  UCHAR_POSIX_GRAPH = 46,
  UCHAR_POSIX_PRINT = 47,
  UCHAR_POSIX_XDIGIT = 48,
  UCHAR_UPPERCASE = 30,
  UCHAR_WHITE_SPACE = 31,
  UCHAR_EAST_ASIAN_WIDTH = 0x1004,
  UCHAR_HANGUL_SYLLABLE_TYPE = 0x100b,
};

enum UCharCategory {
  U_NON_SPACING_MARK = 6,
  U_ENCLOSING_MARK = 7,
  U_CONTROL_CHAR = 15,
  U_FORMAT_CHAR = 16,
};

enum UEastAsianWidth {
  U_EA_NEUTRAL,
  U_EA_AMBIGUOUS,
  U_EA_HALFWIDTH,
  U_EA_FULLWIDTH,
  U_EA_NARROW,
  U_EA_WIDE,
};

enum UHangulSyllableType {
  U_HST_NOT_APPLICABLE,
  U_HST_LEADING_JAMO,
  U_HST_VOWEL_JAMO,
  U_HST_TRAILING_JAMO,
  U_HST_LV_SYLLABLE,
  U_HST_LVT_SYLLABLE,
};

int8_t __icu_charType(wint_t wc);
int32_t __icu_getIntPropertyValue(wint_t wc, UProperty property);
bool __icu_hasBinaryProperty(wint_t wc, UProperty property, int (*fallback)(int));

void* __find_icu_symbol(const char* symbol_name);

#endif  // _PRIVATE_ICU_H
