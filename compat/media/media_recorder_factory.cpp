/*
 * Copyright (C) 2014 Canonical Ltd
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

//#define LOG_NDEBUG 0
#undef LOG_TAG
#define LOG_TAG "MediaRecorderFactory"

#include "media_recorder_factory.h"
#include "media_recorder_client.h"
#include <hybris/media/media_recorder_layer.h>

#include <gui/IGraphicBufferProducer.h>
#include <media/IMediaRecorder.h>
#include <binder/IServiceManager.h>

#include <utils/Log.h>
#include <utils/threads.h>

#define REPORT_FUNCTION() ALOGV("%s \n", __PRETTY_FUNCTION__)

namespace android {

enum {
    CREATE_MEDIA_RECORDER = IBinder::FIRST_CALL_TRANSACTION,
};

class BpMediaRecorderFactory: public BpInterface<IMediaRecorderFactory>
{
public:
    BpMediaRecorderFactory(const sp<IBinder>& impl)
        : BpInterface<IMediaRecorderFactory>(impl)
    {
    }

    virtual sp<IMediaRecorder> createMediaRecorder()
    {
        Parcel data, reply;
        data.writeInterfaceToken(IMediaRecorderFactory::getInterfaceDescriptor());
        remote()->transact(CREATE_MEDIA_RECORDER, data, &reply);
        return interface_cast<IMediaRecorder>(reply.readStrongBinder());
    }
};

// ----------------------------------------------------------------------------

IMPLEMENT_META_INTERFACE(MediaRecorderFactory, "android.media.IMediaRecorderFactory");

// ----------------------------------------------------------------------

status_t BnMediaRecorderFactory::onTransact(
    uint32_t code, const Parcel& data, Parcel* reply, uint32_t flags)
{
    switch (code) {
        case CREATE_MEDIA_RECORDER: {
            CHECK_INTERFACE(IMediaRecorderFactory, data, reply);
            sp<IMediaRecorder> recorder = createMediaRecorder();
            reply->writeStrongBinder(recorder->asBinder());
            return NO_ERROR;
        } break;
        default:
            return BBinder::onTransact(code, data, reply, flags);
    }
}

// ----------------------------------------------------------------------------

sp<MediaRecorderFactory> MediaRecorderFactory::media_recorder_factory;
Mutex MediaRecorderFactory::s_lock;

MediaRecorderFactory::MediaRecorderFactory()
{
    REPORT_FUNCTION();
}

MediaRecorderFactory::~MediaRecorderFactory()
{
    REPORT_FUNCTION();
}

/*!
 * \brief Creates and adds the MediaRecorderFactory service to the default Binder ServiceManager
 */
void MediaRecorderFactory::instantiate()
{
    defaultServiceManager()->addService(
            String16(IMediaRecorderFactory::exported_service_name()), factory_instance());
    ALOGV("Added Binder service '%s' to ServiceManager", IMediaRecorderFactory::exported_service_name());
}

/*!
 * \brief Creates a new MediaRecorderClient instance over Binder
 * \return A new MediaRecorderClient instance
 */
sp<IMediaRecorder> MediaRecorderFactory::createMediaRecorder()
{
    REPORT_FUNCTION();
    sp<MediaRecorderClient> recorder = new MediaRecorderClient();
    return recorder;
}

/*!
 * \brief Get a reference to the MediaRecorderFactory singleton instance
 * \return The MediaRecorderFactory singleton instance
 */
sp<MediaRecorderFactory>& MediaRecorderFactory::factory_instance()
{
    REPORT_FUNCTION();
    Mutex::Autolock _l(s_lock);
    if (media_recorder_factory == NULL)
    {
        ALOGD("Creating new static instance of MediaRecorderFactory");
        media_recorder_factory = new MediaRecorderFactory();
    }

    return media_recorder_factory;
}

} // namespace android
