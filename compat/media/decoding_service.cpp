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

// Uncomment to enable verbose debug output
#define LOG_NDEBUG 0

#undef LOG_TAG
#define LOG_TAG "DecodingService"

#include "decoding_service_priv.h"

#include <binder/IServiceManager.h>
#include <binder/Parcel.h>
#include <binder/IPCThreadState.h>
#include <binder/ProcessState.h>
#include <binder/BpBinder.h>

typedef void* EGLDisplay;
typedef void* EGLSyncKHR;

#include <ui/GraphicBuffer.h>
#include <gui/GraphicBufferAlloc.h>
#include <gui/IGraphicBufferProducer.h>
#include <gui/IGraphicBufferConsumer.h>
#include <gui/Surface.h>
#include <gui/NativeBufferAlloc.h>

namespace android {

IMPLEMENT_META_INTERFACE(DecodingService, "android.media.IDecodingService");
IMPLEMENT_META_INTERFACE(DecodingServiceSession, "android.media.IDecodingServiceSession");

enum {
    GET_IGRAPHICBUFFERCONSUMER = IBinder::FIRST_CALL_TRANSACTION,
    GET_IGRAPHICBUFFERPRODUCER,
    REGISTER_SESSION,
    UNREGISTER_SESSION,
};

// ----------------------------------------------------------------------

status_t BnDecodingService::onTransact(
    uint32_t code, const Parcel& data, Parcel* reply, uint32_t flags)
{
    ALOGD("Entering %s", __PRETTY_FUNCTION__);
    switch (code) {
        case GET_IGRAPHICBUFFERCONSUMER: {
            CHECK_INTERFACE(IDecodingService, data, reply);
            sp<IGraphicBufferConsumer> gbc;
            status_t res = getIGraphicBufferConsumer(&gbc);

            reply->writeStrongBinder(gbc->asBinder());
            reply->writeInt32(res);

            return NO_ERROR;
        } break;
        case GET_IGRAPHICBUFFERPRODUCER: {
            CHECK_INTERFACE(IDecodingService, data, reply);
            sp<IGraphicBufferProducer> gbp;
            status_t res = getIGraphicBufferProducer(&gbp);

            reply->writeStrongBinder(gbp->asBinder());
            reply->writeInt32(res);

            return NO_ERROR;
        } break;
        case REGISTER_SESSION: {
            CHECK_INTERFACE(IDecodingService, data, reply);
            sp<IBinder> binder = data.readStrongBinder();
            sp<IDecodingServiceSession> session(new BpDecodingServiceSession(binder));
            registerSession(session);

            return NO_ERROR;
        } break;
        case UNREGISTER_SESSION: {
            CHECK_INTERFACE(IDecodingService, data, reply);
            unregisterSession();

            return NO_ERROR;
        } break;
        default:
            return BBinder::onTransact(code, data, reply, flags);
    }
}

status_t BpDecodingService::getIGraphicBufferConsumer(sp<IGraphicBufferConsumer>* gbc)
{
    ALOGD("Entering %s", __PRETTY_FUNCTION__);
    Parcel data, reply;
    data.writeInterfaceToken(IDecodingService::getInterfaceDescriptor());
    remote()->transact(GET_IGRAPHICBUFFERCONSUMER, data, &reply);
    *gbc = interface_cast<IGraphicBufferConsumer>(reply.readStrongBinder());
    return reply.readInt32();
}

status_t BpDecodingService::getIGraphicBufferProducer(sp<IGraphicBufferProducer>* gbp)
{
    ALOGD("Entering %s", __PRETTY_FUNCTION__);
    Parcel data, reply;
    data.writeInterfaceToken(IDecodingService::getInterfaceDescriptor());
    remote()->transact(GET_IGRAPHICBUFFERPRODUCER, data, &reply);
    *gbp = interface_cast<IGraphicBufferProducer>(reply.readStrongBinder());
    return NO_ERROR;
}

status_t BpDecodingService::registerSession(const sp<IDecodingServiceSession>& session)
{
    ALOGD("Entering %s", __PRETTY_FUNCTION__);
    Parcel data, reply;
    data.writeInterfaceToken(IDecodingService::getInterfaceDescriptor());
    data.writeStrongBinder(session->asBinder());
    remote()->transact(REGISTER_SESSION, data, &reply);
    return NO_ERROR;
}

status_t BpDecodingService::unregisterSession()
{
    ALOGD("Entering %s", __PRETTY_FUNCTION__);
    Parcel data, reply;
    data.writeInterfaceToken(IDecodingService::getInterfaceDescriptor());
    remote()->transact(UNREGISTER_SESSION, data, &reply);
    return NO_ERROR;
}

sp<DecodingService> DecodingService::decoding_service = NULL;

DecodingService::DecodingService()
    : client_death_cb(NULL),
      client_death_context(NULL)
{
    ALOGD("%s", __PRETTY_FUNCTION__);
}

DecodingService::~DecodingService()
{
    ALOGD("%s", __PRETTY_FUNCTION__);
}

void DecodingService::instantiate()
{
    ALOGD("Entering %s", __PRETTY_FUNCTION__);
    defaultServiceManager()->addService(
            String16(IDecodingService::exported_service_name()), service_instance());
    ALOGD("Added Binder service '%s' to ServiceManager", IDecodingService::exported_service_name());

    service_instance()->createBufferQueue();
}

sp<DecodingService>& DecodingService::service_instance()
{
    ALOGD("Entering %s", __PRETTY_FUNCTION__);

    // TODO Add a mutex here
    if (decoding_service == NULL)
    {
        ALOGD("Creating new static instance of DecodingService");
        decoding_service = new DecodingService();
    }

    return decoding_service;
}

void DecodingService::setDecodingClientDeathCb(DecodingClientDeathCbHybris cb, void* context)
{
    ALOGD("Entering %s", __PRETTY_FUNCTION__);

    client_death_cb = cb;
    client_death_context = context;
}

status_t DecodingService::getIGraphicBufferConsumer(sp<IGraphicBufferConsumer>* gbc)
{
    // TODO: Make sure instantiate() has been called first
    ALOGD("Entering %s", __PRETTY_FUNCTION__);
    pid_t pid = IPCThreadState::self()->getCallingPid();
    ALOGD("Calling Pid: %d", pid);

    *gbc = buffer_queue;

    return OK;
}

status_t DecodingService::getIGraphicBufferProducer(sp<IGraphicBufferProducer>* gbp)
{
    pid_t pid = IPCThreadState::self()->getCallingPid();
    ALOGD("Calling Pid: %d", pid);

    *gbp = buffer_queue;
    ALOGD("producer(gbp): %p", (void*)gbp->get());
    return OK;
}

status_t DecodingService::registerSession(const sp<IDecodingServiceSession>& session)
{
    ALOGD("Entering %s", __PRETTY_FUNCTION__);
    status_t ret = session->asBinder()->linkToDeath(android::sp<android::IBinder::DeathRecipient>(this));
    this->session = session;

    // Create a new BufferQueue instance so that the next created client plays
    // video correctly
    createBufferQueue();

    return ret;
}

status_t DecodingService::unregisterSession()
{
    ALOGD("Entering %s", __PRETTY_FUNCTION__);
    if (session != NULL)
    {
        session->asBinder()->unlinkToDeath(this);
        session.clear();
        // Reset the BufferQueue instance so that the next created client plays
        // video correctly
        buffer_queue.clear();
    }

    return OK;
}

void DecodingService::createBufferQueue()
{
    // Use a new native buffer allocator vs the default one, which means it'll use the proper one
    // that will allow rendering to work with Mir
    sp<IGraphicBufferAlloc> g_buffer_alloc(new GraphicBufferAlloc());

    // This BuferQueue is shared between the client and the service
#if ANDROID_VERSION_MAJOR==4 && ANDROID_VERSION_MINOR<=3
    sp<NativeBufferAlloc> native_alloc(new NativeBufferAlloc());
    buffer_queue = new BufferQueue(false, NULL, native_alloc);
#else
    buffer_queue = new BufferQueue(NULL);
#endif
    ALOGD("buffer_queue: %p", (void*)buffer_queue.get());
    buffer_queue->setBufferCount(5);
}

void DecodingService::binderDied(const wp<IBinder>& who)
{
    ALOGD("Entering %s", __PRETTY_FUNCTION__);
    if (client_death_cb != NULL)
        client_death_cb(client_death_context);

    unregisterSession();
}

sp<BpDecodingService> DecodingClient::decoding_service = NULL;

DecodingClient::DecodingClient()
{
    ALOGD("%s", __PRETTY_FUNCTION__);

    ProcessState::self()->startThreadPool();
}

DecodingClient::~DecodingClient()
{
    ALOGD("%s", __PRETTY_FUNCTION__);
}

sp<BpDecodingService>& DecodingClient::service_instance()
{
    ALOGD("%s", __PRETTY_FUNCTION__);
    // TODO: Add a mutex here
    if (decoding_service == NULL)
    {
        ALOGD("Creating a new static BpDecodingService instance");
        sp<IServiceManager> service_manager = defaultServiceManager();
        sp<IBinder> service = service_manager->getService(
                String16(IDecodingService::exported_service_name()));
        decoding_service = new BpDecodingService(service);
    }

    return decoding_service;
}

status_t DecodingClient::getIGraphicBufferConsumer(sp<IGraphicBufferConsumer>* gbc)
{
    ALOGD("Entering %s", __PRETTY_FUNCTION__);
    return service_instance()->getIGraphicBufferConsumer(gbc);
}

// IDecodingServiceSession

BpDecodingServiceSession::BpDecodingServiceSession(const sp<IBinder>& impl)
    : BpInterface<IDecodingServiceSession>(impl)
{
    ALOGD("%s", __PRETTY_FUNCTION__);
}

BpDecodingServiceSession::~BpDecodingServiceSession()
{
    ALOGD("%s", __PRETTY_FUNCTION__);
}

BnDecodingServiceSession::BnDecodingServiceSession()
{
    ALOGD("%s", __PRETTY_FUNCTION__);
}

BnDecodingServiceSession::~BnDecodingServiceSession()
{
    ALOGD("%s", __PRETTY_FUNCTION__);
}

status_t BnDecodingServiceSession::onTransact(uint32_t code, const Parcel& data,
            Parcel* reply, uint32_t flags)
{
    ALOGD("Entering %s", __PRETTY_FUNCTION__);

    return NO_ERROR;
}

// ----- C API ----- //


}; // namespace android
