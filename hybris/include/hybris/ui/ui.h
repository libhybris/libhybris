/*
 * Copyright (c) 2022 Jolla Ltd.
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

#ifndef hybris_ui_header_include_guard__
#define hybris_ui_header_include_guard__

#include <stdbool.h>
#include <hybris/ui/ui_compatibility_layer.h>

#ifdef __cplusplus
extern "C" {
#endif

void hybris_ui_initialize();
bool hybris_ui_check_for_symbol(const char *sym);

#ifdef __cplusplus
}
#endif

#endif // hybris_ui_header_include_guard__
