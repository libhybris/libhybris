#ifndef NATIVEWINDOWBASE_H
#define NATIVEWINDOWBASE_H

#include <system/window.h>
#include <hardware/gralloc.h>
#include <EGL/egl.h>
#include "support.h"
#include <stdarg.h>

#define NO_ERROR                0L
#define BAD_VALUE               -1

//#define TRACE(...)
#define TRACE printf
/*
 * A Class to do common ANativeBuffer initialization and thunk c-style
 * callbacks into C++ method calls.
 * */


class BaseNativeWindowBuffer : public ANativeWindowBuffer {
public:

protected:
    BaseNativeWindowBuffer() {
        // done in ANativeWindowBuffer::ANativeWindowBuffer
        // common.magic = ANDROID_NATIVE_WINDOW_MAGIC;
        // common.version = sizeof(ANativeWindow);
        // memset(common.reserved, 0, sizeof(window->native.common.reserved));

        ANativeWindowBuffer::common.decRef = &_decRef;
        ANativeWindowBuffer::common.incRef = &_incRef;
        ANativeWindowBuffer::width = 0;
        ANativeWindowBuffer::height = 0;
        ANativeWindowBuffer::stride = 0;
        ANativeWindowBuffer::format = 0;
        ANativeWindowBuffer::usage = 0;
        ANativeWindowBuffer::handle = 0;
    }
private:
    // does this require more magic?
    unsigned int refcount;
    static void _decRef(struct android_native_base_t* base) {
        ANativeWindowBuffer* self = container_of(base, ANativeWindowBuffer, common);
        static_cast<BaseNativeWindowBuffer*>(self)->refcount--;
    };
    static void _incRef(struct android_native_base_t* base) {
        ANativeWindowBuffer* self = container_of(base, ANativeWindowBuffer, common);
        static_cast<BaseNativeWindowBuffer*>(self)->refcount++;
    };
};

/*
 * A Class to do common ANativeWindow initialization and thunk c-style
 * callbacks into C++ method calls.
 * */
class BaseNativeWindow : public ANativeWindow {
public:
    operator EGLNativeWindowType()
    {
        EGLNativeWindowType ret = reinterpret_cast<EGLNativeWindowType>((ANativeWindow*)this);
        TRACE("casting %p to %p\n", this, ret);
        return ret;
    }
protected:
    BaseNativeWindow()
    {
        TRACE("%s this=%p or A %p\n",__PRETTY_FUNCTION__, this, (ANativeWindow*)this);
        // done in ANativeWindow
        // common.magic = ANDROID_NATIVE_WINDOW_MAGIC;
        // common.version = sizeof(ANativeWindow);
        // memset(common.reserved, 0, sizeof(window->native.common.reserved));
        common.decRef = &_decRef;
        common.incRef = &_incRef;

        ANativeWindow::flags = 0;
        ANativeWindow::minSwapInterval = 0;
        ANativeWindow::maxSwapInterval = 0;
        ANativeWindow::xdpi = 0;
        ANativeWindow::ydpi = 0;

        ANativeWindow::setSwapInterval = _setSwapInterval;
        ANativeWindow::lockBuffer = &_lockBuffer;
        ANativeWindow::dequeueBuffer = &_dequeueBuffer;
        ANativeWindow::queueBuffer = &_queueBuffer;
        ANativeWindow::query = &_query;
        ANativeWindow::perform = &_perform;
        ANativeWindow::cancelBuffer = &_cancelBuffer;
        refcount = 0;
    };

    ~BaseNativeWindow()
    {
        TRACE("%s\n",__PRETTY_FUNCTION__);
    };

    // does this require more magic?
    unsigned int refcount;
    static void _decRef(struct android_native_base_t* base) {
        ANativeWindow* self = container_of(base, ANativeWindow, common);
        static_cast<BaseNativeWindow*>(self)->refcount--;
    };
    static void _incRef(struct android_native_base_t* base) {
        ANativeWindow* self = container_of(base, ANativeWindow, common);
        static_cast<BaseNativeWindow*>(self)->refcount++;
    };

    // these have to be implemented in the concrete implementation, eg. FBDEV or offscreen window
    virtual int setSwapInterval(int interval) = 0;
    virtual int dequeueBuffer(BaseNativeWindowBuffer **buffer) = 0;
    virtual int lockBuffer(BaseNativeWindowBuffer* buffer) = 0;
    virtual int queueBuffer(BaseNativeWindowBuffer* buffer) = 0;
    virtual int cancelBuffer(BaseNativeWindowBuffer* buffer) = 0;

    virtual unsigned int type() const = 0;
    virtual unsigned int width() const = 0;
    virtual unsigned int height() const = 0;
    virtual unsigned int format() const = 0;
    virtual unsigned int defaultWidth() const = 0;
    virtual unsigned int defaultHeight() const = 0;
    virtual unsigned int queueLength() const = 0;
    virtual unsigned int transformHint() const = 0;
    //perform interfaces
    virtual int setBuffersFormat(int format) = 0;
    virtual int setBuffersDimensions(int width, int height) = 0;
    virtual int setUsage(int usage) = 0;
private:
    static int _setSwapInterval(struct ANativeWindow* window, int interval)
    {
        TRACE("%s interval=%i\n",__PRETTY_FUNCTION__, interval);
        return static_cast<BaseNativeWindow*>(window)->setSwapInterval(interval);
    }

    static int _dequeueBuffer(ANativeWindow* window, ANativeWindowBuffer** buffer)
    {
        TRACE("%s pointer dest %p, contains %p\n",__PRETTY_FUNCTION__, buffer, *buffer);
        BaseNativeWindowBuffer* temp = static_cast<BaseNativeWindowBuffer*>(*buffer);
        TRACE("temp %p\n", temp);
        int ret = static_cast<BaseNativeWindow*>(window)->dequeueBuffer(&temp);
        TRACE("temp now %p\n", temp);
        *buffer = static_cast<ANativeWindowBuffer*>(temp);
        TRACE("now pointer dest %p, contains %p\n", buffer, *buffer);
        return ret;
    }

    static int _lockBuffer(struct ANativeWindow* window, ANativeWindowBuffer* buffer)
    {
        TRACE("%s buffer=%p\n",__PRETTY_FUNCTION__, buffer);
        return static_cast<BaseNativeWindow*>(window)->lockBuffer(static_cast<BaseNativeWindowBuffer*>(buffer));
    }

    static int _queueBuffer(struct ANativeWindow* window, ANativeWindowBuffer* buffer)
    {
        TRACE("%s buffer=%p\n",__PRETTY_FUNCTION__, buffer);
        return static_cast<BaseNativeWindow*>(window)->queueBuffer(static_cast<BaseNativeWindowBuffer*>(buffer));
        return 0;
    }

    static int _query(const struct ANativeWindow* window, int what, int* value)
    {
        printf("_query window %p %i %p\n", window, what, value);
        const BaseNativeWindow* self=static_cast<const BaseNativeWindow*>(window);
        switch (what) {
            case NATIVE_WINDOW_WIDTH:
                *value = self->width();
                return NO_ERROR;
            case NATIVE_WINDOW_HEIGHT:
                *value = self->height();
                return NO_ERROR;
            case NATIVE_WINDOW_FORMAT:
                *value = self->format();
                printf("done\n");
                return NO_ERROR;
            case NATIVE_WINDOW_CONCRETE_TYPE:
                *value = self->type();
                return NO_ERROR;
            case NATIVE_WINDOW_QUEUES_TO_WINDOW_COMPOSER:
                *value = self->queueLength();
                return NO_ERROR;
            case NATIVE_WINDOW_DEFAULT_WIDTH:
                *value = self->defaultWidth();
                return NO_ERROR;
            case NATIVE_WINDOW_DEFAULT_HEIGHT:
                *value = self->defaultHeight();
                return NO_ERROR;
            case NATIVE_WINDOW_TRANSFORM_HINT:
                *value = self->transformHint();
                return NO_ERROR;
        }
        printf("huh?\n");
        TRACE("EGL error: unkown window attribute!\n");
        *value = 0;
        return BAD_VALUE;
    }

    static int _perform(struct ANativeWindow* window, int operation, ... )
    {
        BaseNativeWindow* self = static_cast<BaseNativeWindow*>(window);
        va_list args;
        va_start(args, operation);

        // FIXME
        TRACE("%s operation = %i\n", __PRETTY_FUNCTION__, operation);
        switch(operation) {
        case NATIVE_WINDOW_SET_USAGE                 : //  0,
        {
            int usage = va_arg(args, int);
            return self->setUsage(usage);
        }
        case NATIVE_WINDOW_CONNECT                   : //  1,   /* deprecated */
            TRACE("connect\n");
            break;
        case NATIVE_WINDOW_DISCONNECT                : //  2,   /* deprecated */
            TRACE("disconnect\n");
            break;
        case NATIVE_WINDOW_SET_CROP                  : //  3,   /* private */
            TRACE("set crop\n");
            break;
        case NATIVE_WINDOW_SET_BUFFER_COUNT          : //  4,
            TRACE("set buffer count\n");
            break;
        case NATIVE_WINDOW_SET_BUFFERS_GEOMETRY      : //  5,   /* deprecated */
            TRACE("set buffers geometry\n");
            break;
        case NATIVE_WINDOW_SET_BUFFERS_TRANSFORM     : //  6,
            TRACE("set buffers transform\n");
            break;
        case NATIVE_WINDOW_SET_BUFFERS_TIMESTAMP     : //  7,
            TRACE("set buffers timestamp\n");
            break;
        case NATIVE_WINDOW_SET_BUFFERS_DIMENSIONS    : //  8,
        {
            int width  = va_arg(args, int);
            int height = va_arg(args, int);
            return self->setBuffersDimensions(width, height);
        }
        case NATIVE_WINDOW_SET_BUFFERS_FORMAT        : //  9,
        {
            int format = va_arg(args, int);
            return self->setBuffersFormat(format);
        }
        case NATIVE_WINDOW_SET_SCALING_MODE          : // 10,   /* private */
            TRACE("set scaling mode\n");
            break;
        case NATIVE_WINDOW_LOCK                      : // 11,   /* private */
            TRACE("window lock\n");
            break;
        case NATIVE_WINDOW_UNLOCK_AND_POST           : // 12,   /* private */
            TRACE("unlock and post\n");
            break;
        case NATIVE_WINDOW_API_CONNECT               : // 13,   /* private */
            TRACE("api connect\n");
            break;
        case NATIVE_WINDOW_API_DISCONNECT            : // 14,   /* private */
            TRACE("api disconnect\n");
            break;
        case NATIVE_WINDOW_SET_BUFFERS_USER_DIMENSIONS : // 15, /* private */
            TRACE("set buffers user dimensions\n");
            break;
        case NATIVE_WINDOW_SET_POST_TRANSFORM_CROP   : // 16,
            TRACE("set post transform crop\n");
            break;
        }
        return NO_ERROR;
    }

    static int _cancelBuffer(struct ANativeWindow* window, ANativeWindowBuffer* buffer)
    {
        TRACE("%s buffer = %p\n",__PRETTY_FUNCTION__, buffer);
        return static_cast<BaseNativeWindow*>(window)->cancelBuffer(static_cast<BaseNativeWindowBuffer*>(buffer));
    }

};

#endif
