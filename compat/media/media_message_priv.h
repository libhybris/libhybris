/*
 * Copyright (C) 2016 Canonical Ltd
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
 * Authored by: Simon Fels <simon.fels@canonical.com>
 */

#ifndef MEDIA_MESSAGE_PRIV_H_
#define MEDIA_MESSAGE_PRIV_H_

#include <hybris/media/media_message_layer.h>

#include <media/stagefright/foundation/AMessage.h>

struct MediaMessagePrivate
{
public:
    static MediaMessagePrivate* toPrivate(MediaMessageWrapper *msg);

    explicit MediaMessagePrivate();
    virtual ~MediaMessagePrivate();

    android::sp<android::AMessage> msg;
};

#endif
