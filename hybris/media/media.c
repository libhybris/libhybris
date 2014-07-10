/*
 * Copyright (C) 2013-2014 Canonical Ltd
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
#include <hybris/media/decoding_service.h>
#include <hybris/media/media_compatibility_layer.h>
#include <hybris/media/media_codec_layer.h>
#include <hybris/media/media_codec_list.h>
#include <hybris/media/media_format_layer.h>
#include <hybris/media/media_recorder_layer.h>
#include <hybris/media/surface_texture_client_hybris.h>

#define COMPAT_LIBRARY_PATH "/system/lib/libmedia_compat_layer.so"

#ifdef __ARM_PCS_VFP
#define FP_ATTRIB __attribute__((pcs("aapcs")))
#else
#define FP_ATTRIB
#endif

HYBRIS_LIBRARY_INITIALIZE(media, COMPAT_LIBRARY_PATH);

int media_compat_check_availability()
{
	/* Both are defined via HYBRIS_LIBRARY_INITIALIZE */
	hybris_media_initialize();
	return media_handle ? 1 : 0;
}

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

// DecodingService
HYBRIS_IMPLEMENT_VOID_FUNCTION0(media, decoding_service_init);
HYBRIS_IMPLEMENT_FUNCTION0(media, IGBCWrapperHybris,
	decoding_service_get_igraphicbufferconsumer);
HYBRIS_IMPLEMENT_FUNCTION0(media, IGraphicBufferProducerHybris,
	decoding_service_get_igraphicbufferproducer);
HYBRIS_IMPLEMENT_FUNCTION0(media, DSSessionWrapperHybris,
	decoding_service_create_session);
HYBRIS_IMPLEMENT_VOID_FUNCTION2(media, decoding_service_set_client_death_cb,
	DecodingClientDeathCbHybris, void*);

// Media Codecs
HYBRIS_IMPLEMENT_FUNCTION1(media, MediaCodecDelegate,
	media_codec_create_by_codec_name, const char*);
HYBRIS_IMPLEMENT_VOID_FUNCTION1(media, media_codec_delegate_destroy,
	MediaCodecDelegate);
HYBRIS_IMPLEMENT_VOID_FUNCTION1(media, media_codec_delegate_ref,
	MediaCodecDelegate);
HYBRIS_IMPLEMENT_VOID_FUNCTION1(media, media_codec_delegate_unref,
	MediaCodecDelegate);

#ifdef SIMPLE_PLAYER
HYBRIS_IMPLEMENT_FUNCTION4(media, int, media_codec_configure,
	MediaCodecDelegate, MediaFormat, void*, uint32_t);
#else
HYBRIS_IMPLEMENT_FUNCTION4(media, int, media_codec_configure,
	MediaCodecDelegate, MediaFormat, SurfaceTextureClientHybris, uint32_t);
#endif
HYBRIS_IMPLEMENT_FUNCTION2(media, int, media_codec_set_surface_texture_client,
	MediaCodecDelegate, SurfaceTextureClientHybris);
HYBRIS_IMPLEMENT_FUNCTION2(media, int, media_codec_queue_csd,
	MediaCodecDelegate, MediaFormat);
HYBRIS_IMPLEMENT_FUNCTION1(media, int, media_codec_start,
	MediaCodecDelegate);
HYBRIS_IMPLEMENT_FUNCTION1(media, int, media_codec_stop,
	MediaCodecDelegate);
HYBRIS_IMPLEMENT_FUNCTION1(media, int, media_codec_release,
	MediaCodecDelegate);
HYBRIS_IMPLEMENT_FUNCTION1(media, int, media_codec_flush,
	MediaCodecDelegate);
HYBRIS_IMPLEMENT_FUNCTION1(media, size_t, media_codec_get_input_buffers_size,
	MediaCodecDelegate);
HYBRIS_IMPLEMENT_FUNCTION2(media, uint8_t*, media_codec_get_nth_input_buffer,
	MediaCodecDelegate, size_t);
HYBRIS_IMPLEMENT_FUNCTION2(media, size_t, media_codec_get_nth_input_buffer_capacity,
	MediaCodecDelegate, size_t);
HYBRIS_IMPLEMENT_FUNCTION1(media, size_t, media_codec_get_output_buffers_size,
	MediaCodecDelegate);
HYBRIS_IMPLEMENT_FUNCTION2(media, uint8_t*, media_codec_get_nth_output_buffer,
	MediaCodecDelegate, size_t);
HYBRIS_IMPLEMENT_FUNCTION2(media, size_t, media_codec_get_nth_output_buffer_capacity,
	MediaCodecDelegate, size_t);
HYBRIS_IMPLEMENT_FUNCTION3(media, int, media_codec_dequeue_output_buffer,
	MediaCodecDelegate, MediaCodecBufferInfo*, int64_t);
HYBRIS_IMPLEMENT_FUNCTION2(media, int, media_codec_queue_input_buffer,
	MediaCodecDelegate, const MediaCodecBufferInfo*);
HYBRIS_IMPLEMENT_FUNCTION3(media, int, media_codec_dequeue_input_buffer,
	MediaCodecDelegate, size_t*, int64_t);
HYBRIS_IMPLEMENT_FUNCTION3(media, int, media_codec_release_output_buffer,
	MediaCodecDelegate, size_t, uint8_t);
HYBRIS_IMPLEMENT_FUNCTION1(media, MediaFormat, media_codec_get_output_format,
	MediaCodecDelegate);

HYBRIS_IMPLEMENT_FUNCTION3(media, ssize_t, media_codec_list_find_codec_by_type,
	const char*, bool, size_t);
HYBRIS_IMPLEMENT_FUNCTION1(media, ssize_t, media_codec_list_find_codec_by_name,
	const char *);
HYBRIS_IMPLEMENT_FUNCTION0(media, size_t, media_codec_list_count_codecs);
HYBRIS_IMPLEMENT_VOID_FUNCTION1(media, media_codec_list_get_codec_info_at_id,
	size_t);
HYBRIS_IMPLEMENT_FUNCTION1(media, const char*, media_codec_list_get_codec_name,
	size_t);
HYBRIS_IMPLEMENT_FUNCTION1(media, bool, media_codec_list_is_encoder,
	size_t);
HYBRIS_IMPLEMENT_FUNCTION1(media, size_t, media_codec_list_get_num_supported_types,
	size_t);
HYBRIS_IMPLEMENT_FUNCTION2(media, size_t, media_codec_list_get_nth_supported_type_len,
	size_t, size_t);
HYBRIS_IMPLEMENT_FUNCTION3(media, int, media_codec_list_get_nth_supported_type,
	size_t, char *, size_t);
HYBRIS_IMPLEMENT_FUNCTION2(media, size_t, media_codec_list_get_num_profile_levels,
	size_t, const char*);
HYBRIS_IMPLEMENT_FUNCTION2(media, size_t, media_codec_list_get_num_color_formats,
	size_t, const char*);
HYBRIS_IMPLEMENT_FUNCTION4(media, int, media_codec_list_get_nth_codec_profile_level,
	size_t, const char*, profile_level*, size_t);
HYBRIS_IMPLEMENT_FUNCTION3(media, int, media_codec_list_get_codec_color_formats,
	size_t, const char*, uint32_t*);

HYBRIS_IMPLEMENT_FUNCTION5(media, MediaFormat, media_format_create_video_format,
	const char*, int32_t, int32_t, int64_t, int32_t);
HYBRIS_IMPLEMENT_VOID_FUNCTION1(media, media_format_destroy,
	MediaFormat);
HYBRIS_IMPLEMENT_VOID_FUNCTION1(media, media_format_ref,
	MediaFormat);
HYBRIS_IMPLEMENT_VOID_FUNCTION1(media, media_format_unref,
	MediaFormat);
HYBRIS_IMPLEMENT_VOID_FUNCTION4(media, media_format_set_byte_buffer,
	MediaFormat, const char*, uint8_t*, size_t);
HYBRIS_IMPLEMENT_FUNCTION1(media, const char*, media_format_get_mime,
	MediaFormat);
HYBRIS_IMPLEMENT_FUNCTION1(media, int64_t, media_format_get_duration_us,
	MediaFormat);
HYBRIS_IMPLEMENT_FUNCTION1(media, int32_t, media_format_get_width,
	MediaFormat);
HYBRIS_IMPLEMENT_FUNCTION1(media, int32_t, media_format_get_height,
	MediaFormat);
HYBRIS_IMPLEMENT_FUNCTION1(media, int32_t, media_format_get_max_input_size,
	MediaFormat);
HYBRIS_IMPLEMENT_FUNCTION1(media, int32_t, media_format_get_stride,
	MediaFormat);
HYBRIS_IMPLEMENT_FUNCTION1(media, int32_t, media_format_get_slice_height,
	MediaFormat);
HYBRIS_IMPLEMENT_FUNCTION1(media, int32_t, media_format_get_color_format,
	MediaFormat);
HYBRIS_IMPLEMENT_FUNCTION1(media, int32_t, media_format_get_crop_left,
	MediaFormat);
HYBRIS_IMPLEMENT_FUNCTION1(media, int32_t, media_format_get_crop_right,
	MediaFormat);
HYBRIS_IMPLEMENT_FUNCTION1(media, int32_t, media_format_get_crop_top,
	MediaFormat);
HYBRIS_IMPLEMENT_FUNCTION1(media, int32_t, media_format_get_crop_bottom,
	MediaFormat);

// SurfaceTextureClientHybris
HYBRIS_IMPLEMENT_FUNCTION1(media, SurfaceTextureClientHybris,
	surface_texture_client_create, EGLNativeWindowType);
HYBRIS_IMPLEMENT_FUNCTION1(media, SurfaceTextureClientHybris,
	surface_texture_client_create_by_id, unsigned int);
HYBRIS_IMPLEMENT_FUNCTION1(media, SurfaceTextureClientHybris,
	surface_texture_client_create_by_igbp, IGBPWrapperHybris);
HYBRIS_IMPLEMENT_FUNCTION2(media, GLConsumerWrapperHybris,
	gl_consumer_create_by_id_with_igbc, unsigned int, IGBCWrapperHybris);
HYBRIS_IMPLEMENT_FUNCTION3(media, int,
	gl_consumer_set_frame_available_cb, GLConsumerWrapperHybris, FrameAvailableCbHybris, void*);
HYBRIS_IMPLEMENT_VOID_FUNCTION2(media, gl_consumer_get_transformation_matrix,
	GLConsumerWrapperHybris, GLfloat*);
HYBRIS_IMPLEMENT_VOID_FUNCTION1(media, gl_consumer_update_texture,
	GLConsumerWrapperHybris);
HYBRIS_IMPLEMENT_FUNCTION1(media, uint8_t,
	surface_texture_client_is_ready_for_rendering, SurfaceTextureClientHybris);
HYBRIS_IMPLEMENT_FUNCTION1(media, uint8_t,
	surface_texture_client_hardware_rendering, SurfaceTextureClientHybris);
HYBRIS_IMPLEMENT_VOID_FUNCTION2(media, surface_texture_client_set_hardware_rendering,
	SurfaceTextureClientHybris, uint8_t);
HYBRIS_IMPLEMENT_VOID_FUNCTION2(media, surface_texture_client_get_transformation_matrix,
	SurfaceTextureClientHybris, GLfloat*);
HYBRIS_IMPLEMENT_VOID_FUNCTION1(media, surface_texture_client_update_texture,
	SurfaceTextureClientHybris);
HYBRIS_IMPLEMENT_VOID_FUNCTION1(media, surface_texture_client_destroy,
	SurfaceTextureClientHybris);
HYBRIS_IMPLEMENT_VOID_FUNCTION1(media, surface_texture_client_ref,
	SurfaceTextureClientHybris);
HYBRIS_IMPLEMENT_VOID_FUNCTION1(media, surface_texture_client_unref,
	SurfaceTextureClientHybris);
HYBRIS_IMPLEMENT_VOID_FUNCTION2(media, surface_texture_client_set_surface_texture,
	SurfaceTextureClientHybris, EGLNativeWindowType);

// Recorder
HYBRIS_IMPLEMENT_FUNCTION0(media, struct MediaRecorderWrapper*,
	android_media_new_recorder);
HYBRIS_IMPLEMENT_FUNCTION1(media, int, android_recorder_initCheck,
	struct MediaRecorderWrapper*);
HYBRIS_IMPLEMENT_FUNCTION2(media, int, android_recorder_setCamera,
	struct MediaRecorderWrapper*, struct CameraControl*);
HYBRIS_IMPLEMENT_FUNCTION2(media, int, android_recorder_setVideoSource,
	struct MediaRecorderWrapper*, VideoSource);
HYBRIS_IMPLEMENT_FUNCTION2(media, int, android_recorder_setAudioSource,
	struct MediaRecorderWrapper*, AudioSource);
HYBRIS_IMPLEMENT_FUNCTION2(media, int, android_recorder_setOutputFormat,
	struct MediaRecorderWrapper*, OutputFormat);
HYBRIS_IMPLEMENT_FUNCTION2(media, int, android_recorder_setVideoEncoder,
	struct MediaRecorderWrapper*, VideoEncoder);
HYBRIS_IMPLEMENT_FUNCTION2(media, int, android_recorder_setAudioEncoder,
	struct MediaRecorderWrapper*, AudioEncoder);
HYBRIS_IMPLEMENT_FUNCTION2(media, int, android_recorder_setOutputFile,
	struct MediaRecorderWrapper*, int);
HYBRIS_IMPLEMENT_FUNCTION3(media, int, android_recorder_setVideoSize,
	struct MediaRecorderWrapper*, int, int);
HYBRIS_IMPLEMENT_FUNCTION2(media, int, android_recorder_setVideoFrameRate,
	struct MediaRecorderWrapper*, int);
HYBRIS_IMPLEMENT_FUNCTION2(media, int, android_recorder_setParameters,
	struct MediaRecorderWrapper*, const char*);
HYBRIS_IMPLEMENT_FUNCTION1(media, int, android_recorder_start,
	struct MediaRecorderWrapper*);
HYBRIS_IMPLEMENT_FUNCTION1(media, int, android_recorder_stop,
	struct MediaRecorderWrapper*);
HYBRIS_IMPLEMENT_FUNCTION1(media, int, android_recorder_prepare,
	struct MediaRecorderWrapper*);
HYBRIS_IMPLEMENT_FUNCTION1(media, int, android_recorder_reset,
	struct MediaRecorderWrapper*);
HYBRIS_IMPLEMENT_FUNCTION1(media, int, android_recorder_close,
	struct MediaRecorderWrapper*);
HYBRIS_IMPLEMENT_FUNCTION1(media, int, android_recorder_release,
	struct MediaRecorderWrapper*);

// Recorder Callbacks
HYBRIS_IMPLEMENT_VOID_FUNCTION3(media, android_recorder_set_error_cb,
	struct MediaRecorderWrapper*, on_recorder_msg_error, void*);
HYBRIS_IMPLEMENT_VOID_FUNCTION3(media, android_recorder_set_audio_read_cb,
	struct MediaRecorderWrapper*, on_recorder_read_audio, void*);
