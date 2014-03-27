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

#ifndef DECODING_SERVICE_PRIV_H_
#define DECODING_SERVICE_PRIV_H_

#include "surface_texture_client_hybris_priv.h"

#include <binder/IInterface.h>
#include <binder/Parcel.h>

namespace android {

class IGraphicBufferConsumer;
class IGraphicBufferProducer;
class BufferQueue;
class GLConsumer;

class IDecodingServiceSession : public IInterface
{
public:
    DECLARE_META_INTERFACE(DecodingServiceSession);

    static const char* exported_service_name() { return "android.media.IDecodingServiceSession"; }

};

class BnDecodingServiceSession : public BnInterface<IDecodingServiceSession>
{
public:
    BnDecodingServiceSession();
    virtual ~BnDecodingServiceSession();

    virtual status_t onTransact(uint32_t code, const Parcel& data,
                                Parcel* reply, uint32_t flags = 0);
};

enum {
    SET_DECODING_CLIENT_DEATH_CB = IBinder::FIRST_CALL_TRANSACTION,
};

class BpDecodingServiceSession : public BpInterface<IDecodingServiceSession>
{
public:
    BpDecodingServiceSession(const sp<IBinder>& impl);
    ~BpDecodingServiceSession();
};

class IDecodingService: public IInterface
{
public:
    DECLARE_META_INTERFACE(DecodingService);

    static const char* exported_service_name() { return "android.media.IDecodingService"; }

    virtual status_t getIGraphicBufferConsumer(sp<IGraphicBufferConsumer>* gbc) = 0;
    virtual status_t getIGraphicBufferProducer(sp<IGraphicBufferProducer>* gbp) = 0;
    virtual status_t registerSession(const sp<IDecodingServiceSession>& session) = 0;
    virtual status_t unregisterSession() = 0;
};

class BnDecodingService: public BnInterface<IDecodingService>
{
public:
    virtual status_t onTransact( uint32_t code,
                                 const Parcel& data,
                                 Parcel* reply,
                                 uint32_t flags = 0);
};

class BpDecodingService: public BpInterface<IDecodingService>
{
public:
    BpDecodingService(const sp<IBinder>& impl)
        : BpInterface<IDecodingService>(impl)
    {
        ALOGD("Entering %s", __PRETTY_FUNCTION__);
    }

    virtual status_t getIGraphicBufferConsumer(sp<IGraphicBufferConsumer>* gbc);
    virtual status_t getIGraphicBufferProducer(sp<IGraphicBufferProducer>* gbp);
    virtual status_t registerSession(const sp<IDecodingServiceSession>& session);
    virtual status_t unregisterSession();
};

class DecodingService : public BnDecodingService,
                        public IBinder::DeathRecipient
{
public:
    DecodingService();
    virtual ~DecodingService();
    /** Adds the decoding service to the default service manager in Binder **/
    static void instantiate();
    static sp<DecodingService>& service_instance();

    virtual void setDecodingClientDeathCb(DecodingClientDeathCbHybris cb, void* context);

    // IDecodingService interface:
    virtual status_t getIGraphicBufferConsumer(sp<IGraphicBufferConsumer>* gbc);
    virtual status_t getIGraphicBufferProducer(sp<IGraphicBufferProducer>* gbp);

    virtual status_t registerSession(const sp<IDecodingServiceSession>& session);
    virtual status_t unregisterSession();

    /** Get notified when the Binder connection to the client dies **/
    virtual void binderDied(const wp<IBinder>& who);

protected:
    virtual void createBufferQueue();

private:
    static sp<DecodingService> decoding_service;
    sp<BufferQueue> buffer_queue;
    sp<IDecodingServiceSession> session;
    DecodingClientDeathCbHybris client_death_cb;
    void *client_death_context;
};

class DecodingClient
{
public:
    DecodingClient();
    virtual ~DecodingClient();

    static sp<BpDecodingService>& service_instance();

    virtual status_t getIGraphicBufferConsumer(sp<IGraphicBufferConsumer>* gbc);

private:
    static sp<BpDecodingService> decoding_service;
};

struct IGBCWrapper
{
    IGBCWrapper(const sp<IGraphicBufferConsumer>& igbc)
    {
        consumer = igbc;
    }

    sp<IGraphicBufferConsumer> consumer;
};

struct IGBPWrapper
{
    IGBPWrapper(const sp<IGraphicBufferProducer>& igbp)
    {
        producer = igbp;
    }

    sp<IGraphicBufferProducer> producer;
};

struct GLConsumerWrapper
{
    GLConsumerWrapper(const sp<_GLConsumerHybris>& gl_consumer)
    {
        consumer = gl_consumer;
    }

    sp<_GLConsumerHybris> consumer;
};

struct DSSessionWrapper
{
    DSSessionWrapper(const sp<IDecodingServiceSession>& session)
    {
        ALOGD("Entering %s", __PRETTY_FUNCTION__);
        this->session = session;
    }

    ~DSSessionWrapper()
    {
        ALOGD("Entering %s", __PRETTY_FUNCTION__);
    }

    sp<IDecodingServiceSession> session;
};

}; // namespace android

#endif
