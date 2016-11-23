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
 */

// Uncomment to enable verbose debug output
#define LOG_NDEBUG 0

#undef LOG_TAG
#define LOG_TAG "MediaCodecList"

#include <hybris/media/media_codec_list.h>

#include <media/stagefright/foundation/AString.h>
#include <media/stagefright/MediaCodecList.h>

#include <utils/Log.h>
#include <utils/Vector.h>

#define REPORT_FUNCTION() ALOGV("%s \n", __PRETTY_FUNCTION__);

using namespace android;

ssize_t media_codec_list_find_codec_by_type(const char *type, bool encoder, size_t startIndex)
{
    REPORT_FUNCTION()
    return MediaCodecList::getInstance()->findCodecByType(type, encoder, startIndex);
}

ssize_t media_codec_list_find_codec_by_name(const char *name)
{
    REPORT_FUNCTION()
    return MediaCodecList::getInstance()->findCodecByName(name);
}

size_t media_codec_list_count_codecs()
{
    REPORT_FUNCTION()
    return MediaCodecList::getInstance()->countCodecs();
}

void media_codec_list_get_codec_info_at_id(size_t index)
{
    REPORT_FUNCTION()
}

const char *media_codec_list_get_codec_name(size_t index)
{
    REPORT_FUNCTION()
#if ANDROID_VERSION_MAJOR>=5
    return MediaCodecList::getInstance()->getCodecInfo(index)->getCodecName();
#else
    return MediaCodecList::getInstance()->getCodecName(index);
#endif
}

bool media_codec_list_is_encoder(size_t index)
{
    REPORT_FUNCTION()
#if ANDROID_VERSION_MAJOR>=5
    return MediaCodecList::getInstance()->getCodecInfo(index)->isEncoder();
#else
    return MediaCodecList::getInstance()->isEncoder(index);
#endif
}

size_t media_codec_list_get_num_supported_types(size_t index)
{
    REPORT_FUNCTION()

    Vector<AString> types;
#if ANDROID_VERSION_MAJOR>=5
    MediaCodecList::getInstance()->getCodecInfo(index)->getSupportedMimes(&types);
#else
    status_t err = MediaCodecList::getInstance()->getSupportedTypes(index, &types);
    if (err != OK)
    {
        ALOGE("Failed to get the number of supported codec types (err: %d)", err);
        return 0;
    }
#endif
    ALOGD("Number of supported codec types: %d", types.size());

    return types.size();
}

size_t media_codec_list_get_nth_supported_type_len(size_t index, size_t n)
{
    REPORT_FUNCTION()

    Vector<AString> types;
#if ANDROID_VERSION_MAJOR>=5
    MediaCodecList::getInstance()->getCodecInfo(index)->getSupportedMimes(&types);
#else
    status_t err = MediaCodecList::getInstance()->getSupportedTypes(index, &types);
#endif

    return types[n].size();
}

int media_codec_list_get_nth_supported_type(size_t index, char *type, size_t n)
{
    REPORT_FUNCTION()

    if (type == NULL)
    {
        ALOGE("types must not be NULL");
        return BAD_VALUE;
    }

    Vector<AString> types;
#if ANDROID_VERSION_MAJOR>=5
    MediaCodecList::getInstance()->getCodecInfo(index)->getSupportedMimes(&types);
#else
    status_t err = MediaCodecList::getInstance()->getSupportedTypes(index, &types);
#endif
    for (size_t i=0; i<types[n].size(); ++i)
        type[i] = types.itemAt(n).c_str()[i];

#if ANDROID_VERSION_MAJOR>=5
    return OK;
#else
    return err;
#endif
}

static void media_codec_list_get_num_codec_capabilities(size_t index, const char *type, size_t *num_profile_levels, size_t *num_color_formats)
{
    REPORT_FUNCTION()

#if ANDROID_VERSION_MAJOR>=5
    Vector<MediaCodecInfo::ProfileLevel> profile_levels;
#else
    Vector<MediaCodecList::ProfileLevel> profile_levels;
#endif
    Vector<uint32_t> color_formats;
    ALOGD("index: %d, type: '%s'", index, type);
#if ANDROID_VERSION_MAJOR>=5
    MediaCodecList::getInstance()->getCodecInfo(index)->getCapabilitiesFor(type)->getSupportedProfileLevels(&profile_levels);
    MediaCodecList::getInstance()->getCodecInfo(index)->getCapabilitiesFor(type)->getSupportedColorFormats(&color_formats);
#elif ANDROID_VERSION_MAJOR==4 && ANDROID_VERSION_MINOR<=3
    status_t err = MediaCodecList::getInstance()->getCodecCapabilities(index, type, &profile_levels, &color_formats);
#else
    uint32_t flags;
    status_t err = MediaCodecList::getInstance()->getCodecCapabilities(index, type, &profile_levels, &color_formats, &flags);
#endif
#if ANDROID_VERSION_MAJOR<5
    if (err != OK)
    {
        ALOGE("Failed to get the number of supported codec capabilities (err: %d)", err);
        return;
    }
#endif

    if (num_profile_levels != NULL)
    {
        ALOGD("Number of codec profile levels: %d", profile_levels.size());
        *num_profile_levels = profile_levels.size();
    }
    if (num_color_formats != NULL)
    {
        ALOGD("Number of codec color formats: %d", color_formats.size());
        *num_color_formats = color_formats.size();
    }
}

size_t media_codec_list_get_num_profile_levels(size_t index, const char *type)
{
    REPORT_FUNCTION()

    size_t num = 0;
    media_codec_list_get_num_codec_capabilities(index, type, &num, NULL);

    return num;
}

size_t media_codec_list_get_num_color_formats(size_t index, const char *type)
{
    REPORT_FUNCTION()

    size_t num = 0;
    media_codec_list_get_num_codec_capabilities(index, type, NULL, &num);

    return num;
}

int media_codec_list_get_nth_codec_profile_level(size_t index, const char *type, profile_level *pro_level, size_t n)
{
    REPORT_FUNCTION()

    if (type == NULL)
    {
        ALOGE("types must not be NULL");
        return BAD_VALUE;
    }

    if (pro_level == NULL)
    {
        ALOGE("pro_level must not be NULL");
        return BAD_VALUE;
    }

#if ANDROID_VERSION_MAJOR>=5
    Vector<MediaCodecInfo::ProfileLevel> profile_levels;
#else
    Vector<MediaCodecList::ProfileLevel> profile_levels;
#endif
    Vector<uint32_t> formats;
#if ANDROID_VERSION_MAJOR>=5
    MediaCodecList::getInstance()->getCodecInfo(index)->getCapabilitiesFor(type)->getSupportedProfileLevels(&profile_levels);
#elif ANDROID_VERSION_MAJOR==4 && ANDROID_VERSION_MINOR<=3
    status_t err = MediaCodecList::getInstance()->getCodecCapabilities(index, type, &profile_levels, &formats);
#else
    uint32_t flags;
    status_t err = MediaCodecList::getInstance()->getCodecCapabilities(index, type, &profile_levels, &formats, &flags);
#endif
#if ANDROID_VERSION_MAJOR<5
    if (err != OK)
    {
        ALOGE("Failed to get the nth codec profile level (err: %d)", err);
        return 0;
    }
#endif

    pro_level->profile = profile_levels[n].mProfile;
    pro_level->level = profile_levels[n].mLevel;

#if ANDROID_VERSION_MAJOR>=5
    return OK;
#else
    return err;
#endif
}

int media_codec_list_get_codec_color_formats(size_t index, const char *type, uint32_t *color_formats)
{
    REPORT_FUNCTION()

#if ANDROID_VERSION_MAJOR>=5
    Vector<MediaCodecInfo::ProfileLevel> profile_levels;
#else
    Vector<MediaCodecList::ProfileLevel> profile_levels;
#endif
    Vector<uint32_t> formats;
#if ANDROID_VERSION_MAJOR>=5
    MediaCodecList::getInstance()->getCodecInfo(index)->getCapabilitiesFor(type)->getSupportedColorFormats(&formats);
#elif ANDROID_VERSION_MAJOR==4 && ANDROID_VERSION_MINOR<=3
    status_t err = MediaCodecList::getInstance()->getCodecCapabilities(index, type, &profile_levels, &formats);
#else
    uint32_t flags;
    status_t err = MediaCodecList::getInstance()->getCodecCapabilities(index, type, &profile_levels, &formats, &flags);
#endif
#if ANDROID_VERSION_MAJOR<5
    if (err != OK)
    {
        ALOGE("Failed to get the number of supported codec types (err: %d)", err);
        return 0;
    }
#endif

    for (size_t i=0; i<formats.size(); ++i)
    {
        color_formats[i] = formats[i];
        ALOGD("Color format [%d]: %d", i, formats[i]);
    }

    return OK;
}
