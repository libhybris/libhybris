#include <android/system/window.h>
#include <android/hardware/gralloc.h>
#include "support.h"
#include <stdarg.h>

#include "nativewindowbase.h"

BaseNativeWindowBuffer::BaseNativeWindowBuffer()
{
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

void BaseNativeWindowBuffer::_decRef(struct android_native_base_t* base)
{
	ANativeWindowBuffer* self = container_of(base, ANativeWindowBuffer, common);
	static_cast<BaseNativeWindowBuffer*>(self)->refcount--;
};

void BaseNativeWindowBuffer::_incRef(struct android_native_base_t* base)
{
	ANativeWindowBuffer* self = container_of(base, ANativeWindowBuffer, common);
	static_cast<BaseNativeWindowBuffer*>(self)->refcount++;
};

BaseNativeWindow::BaseNativeWindow()
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

BaseNativeWindow::~BaseNativeWindow()
{
	TRACE("%s\n",__PRETTY_FUNCTION__);
};

void BaseNativeWindow::_decRef(struct android_native_base_t* base)
{
	ANativeWindow* self = container_of(base, ANativeWindow, common);
	static_cast<BaseNativeWindow*>(self)->refcount--;
};

void BaseNativeWindow::_incRef(struct android_native_base_t* base)
{
	ANativeWindow* self = container_of(base, ANativeWindow, common);
	static_cast<BaseNativeWindow*>(self)->refcount++;
};

int BaseNativeWindow::_setSwapInterval(struct ANativeWindow* window, int interval)
{
	TRACE("%s interval=%i\n",__PRETTY_FUNCTION__, interval);
	return static_cast<BaseNativeWindow*>(window)->setSwapInterval(interval);
}

int BaseNativeWindow::_dequeueBuffer(ANativeWindow* window, ANativeWindowBuffer** buffer)
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

const char *BaseNativeWindow::_native_window_operation(int what)
{
	switch (what) {
		case NATIVE_WINDOW_SET_USAGE: return "NATIVE_WINDOW_SET_USAGE";
		case NATIVE_WINDOW_CONNECT: return "NATIVE_WINDOW_CONNECT";
		case NATIVE_WINDOW_DISCONNECT: return "NATIVE_WINDOW_DISCONNECT";
		case NATIVE_WINDOW_SET_CROP: return "NATIVE_WINDOW_SET_CROP";
		case NATIVE_WINDOW_SET_BUFFER_COUNT: return "NATIVE_WINDOW_SET_BUFFER_COUNT";
		case NATIVE_WINDOW_SET_BUFFERS_GEOMETRY: return "NATIVE_WINDOW_SET_BUFFERS_GEOMETRY";
		case NATIVE_WINDOW_SET_BUFFERS_TRANSFORM: return "NATIVE_WINDOW_SET_BUFFERS_TRANSFORM";
		case NATIVE_WINDOW_SET_BUFFERS_TIMESTAMP: return "NATIVE_WINDOW_SET_BUFFERS_TIMESTAMP";
		case NATIVE_WINDOW_SET_BUFFERS_DIMENSIONS: return "NATIVE_WINDOW_SET_BUFFERS_DIMENSIONS";
		case NATIVE_WINDOW_SET_BUFFERS_FORMAT: return "NATIVE_WINDOW_SET_BUFFERS_FORMAT";
		case NATIVE_WINDOW_SET_SCALING_MODE: return "NATIVE_WINDOW_SET_SCALING_MODE";
		case NATIVE_WINDOW_LOCK: return "NATIVE_WINDOW_LOCK";
		case NATIVE_WINDOW_UNLOCK_AND_POST: return "NATIVE_WINDOW_UNLOCK_AND_POST";
		case NATIVE_WINDOW_API_CONNECT: return "NATIVE_WINDOW_API_CONNECT";
		case NATIVE_WINDOW_API_DISCONNECT: return "NATIVE_WINDOW_API_DISCONNECT";
		case NATIVE_WINDOW_SET_BUFFERS_USER_DIMENSIONS: return "NATIVE_WINDOW_SET_BUFFERS_USER_DIMENSIONS";
		case NATIVE_WINDOW_SET_POST_TRANSFORM_CROP: return "NATIVE_WINDOW_SET_POST_TRANSFORM_CROP";
		default: return "NATIVE_UNKNOWN_OPERATION";
	}
}
const char *BaseNativeWindow::_native_query_operation(int what)
{
	switch (what) {
		case NATIVE_WINDOW_WIDTH: return "NATIVE_WINDOW_WIDTH";
		case NATIVE_WINDOW_HEIGHT: return "NATIVE_WINDOW_HEIGHT";
		case NATIVE_WINDOW_FORMAT: return "NATIVE_WINDOW_FORMAT";
		case NATIVE_WINDOW_MIN_UNDEQUEUED_BUFFERS: return "NATIVE_WINDOW_MIN_UNDEQUEUED_BUFFERS";
		case NATIVE_WINDOW_QUEUES_TO_WINDOW_COMPOSER: return "NATIVE_WINDOW_QUEUES_TO_WINDOW_COMPOSER";
		case NATIVE_WINDOW_CONCRETE_TYPE: return "NATIVE_WINDOW_CONCRETE_TYPE";
		case NATIVE_WINDOW_DEFAULT_WIDTH: return "NATIVE_WINDOW_DEFAULT_WIDTH";
		case NATIVE_WINDOW_DEFAULT_HEIGHT: return "NATIVE_WINDOW_DEFAULT_HEIGHT";
		case NATIVE_WINDOW_TRANSFORM_HINT: return "NATIVE_WINDOW_TRANSFORM_HINT";
		case NATIVE_WINDOW_CONSUMER_RUNNING_BEHIND: return "NATIVE_WINDOW_CONSUMER_RUNNING_BEHIND";
		default: return "NATIVE_UNKNOWN_QUERY";
	}
}

int BaseNativeWindow::_lockBuffer(struct ANativeWindow* window, ANativeWindowBuffer* buffer)
{
	TRACE("%s buffer=%p\n",__PRETTY_FUNCTION__, buffer);
	return static_cast<BaseNativeWindow*>(window)->lockBuffer(static_cast<BaseNativeWindowBuffer*>(buffer));
}

int BaseNativeWindow::_queueBuffer(struct ANativeWindow* window, ANativeWindowBuffer* buffer)
{
	TRACE("%s buffer=%p\n",__PRETTY_FUNCTION__, buffer);
	return static_cast<BaseNativeWindow*>(window)->queueBuffer(static_cast<BaseNativeWindowBuffer*>(buffer));
	return 0;
}

int BaseNativeWindow::_query(const struct ANativeWindow* window, int what, int* value)
{
	TRACE("_query window %p %i %s %p\n", window, what, _native_query_operation(what), value);
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
		case NATIVE_WINDOW_MIN_UNDEQUEUED_BUFFERS:
			*value = 1;
			return NO_ERROR;
	}
	TRACE("EGL error: unkown window attribute! %i\n", what);
	*value = 0;
	return BAD_VALUE;
}

int BaseNativeWindow::_perform(struct ANativeWindow* window, int operation, ... )
{
	BaseNativeWindow* self = static_cast<BaseNativeWindow*>(window);
	va_list args;
	va_start(args, operation);

	// FIXME
	TRACE("%s operation = %s\n", __PRETTY_FUNCTION__, _native_window_operation(operation));
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

int BaseNativeWindow::_cancelBuffer(struct ANativeWindow* window, ANativeWindowBuffer* buffer)
{
	TRACE("%s buffer = %p\n",__PRETTY_FUNCTION__, buffer);
	return static_cast<BaseNativeWindow*>(window)->cancelBuffer(static_cast<BaseNativeWindowBuffer*>(buffer));
}

