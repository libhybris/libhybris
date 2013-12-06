/*
 * Copyright (C) 2013 Canonical Ltd
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
 *
 * Authored by: Jim Hodapp <jim.hodapp@canonical.com>
 *              Ricardo Salveti de Araujo <ricardo.salveti@canonical.com>
 */

#include <assert.h>
#include <dlfcn.h>
#include <stddef.h>
#include <stdbool.h>

#include <hybris/internal/binding.h>
#include <hybris/media/media_compatibility_layer.h>

#define COMPAT_LIBRARY_PATH "/system/lib/libmedia_compat_layer.so"

#ifdef __ARM_PCS_VFP
#define FP_ATTRIB __attribute__((pcs("aapcs")))
#else
#define FP_ATTRIB
#endif

HYBRIS_LIBRARY_INITIALIZE(media, COMPAT_LIBRARY_PATH);

HYBRIS_IMPLEMENT_FUNCTION0(media, struct MediaPlayerWrapper*,
	android_media_new_player);
HYBRIS_IMPLEMENT_VOID_FUNCTION1(media, android_media_update_surface_texture,
	struct MediaPlayerWrapper*);
HYBRIS_IMPLEMENT_FUNCTION1(media, int, android_media_play,
	struct MediaPlayerWrapper*);
HYBRIS_IMPLEMENT_FUNCTION1(media, int, android_media_pause,
	struct MediaPlayerWrapper*);
HYBRIS_IMPLEMENT_FUNCTION1(media, int, android_media_stop,
	struct MediaPlayerWrapper*);
HYBRIS_IMPLEMENT_FUNCTION1(media, bool, android_media_is_playing,
	struct MediaPlayerWrapper*);
HYBRIS_IMPLEMENT_FUNCTION2(media, int, android_media_seek_to,
	struct MediaPlayerWrapper*, int);

// Setters
HYBRIS_IMPLEMENT_FUNCTION2(media, int, android_media_set_data_source,
	struct MediaPlayerWrapper*, const char*);
HYBRIS_IMPLEMENT_FUNCTION2(media, int, android_media_set_preview_texture,
	struct MediaPlayerWrapper*, int);
HYBRIS_IMPLEMENT_FUNCTION2(media, int, android_media_set_volume,
	struct MediaPlayerWrapper*, int);

// Getters
HYBRIS_IMPLEMENT_VOID_FUNCTION2(media, android_media_surface_texture_get_transformation_matrix,
	struct MediaPlayerWrapper*, float*);
HYBRIS_IMPLEMENT_FUNCTION2(media, int, android_media_get_current_position,
	struct MediaPlayerWrapper*, int*);
HYBRIS_IMPLEMENT_FUNCTION2(media, int, android_media_get_duration,
	struct MediaPlayerWrapper*, int*);
HYBRIS_IMPLEMENT_FUNCTION2(media, int, android_media_get_volume,
	struct MediaPlayerWrapper*, int*);

// Callbacks
HYBRIS_IMPLEMENT_VOID_FUNCTION3(media, android_media_set_video_size_cb,
	struct MediaPlayerWrapper*, on_msg_set_video_size, void*);
HYBRIS_IMPLEMENT_VOID_FUNCTION3(media, android_media_set_video_texture_needs_update_cb,
	struct MediaPlayerWrapper*, on_video_texture_needs_update, void*);
HYBRIS_IMPLEMENT_VOID_FUNCTION3(media, android_media_set_error_cb,
	struct MediaPlayerWrapper*, on_msg_error, void*);
HYBRIS_IMPLEMENT_VOID_FUNCTION3(media, android_media_set_playback_complete_cb,
	struct MediaPlayerWrapper*, on_playback_complete, void*);
HYBRIS_IMPLEMENT_VOID_FUNCTION3(media, android_media_set_media_prepared_cb,
	struct MediaPlayerWrapper*, on_media_prepared, void*);
