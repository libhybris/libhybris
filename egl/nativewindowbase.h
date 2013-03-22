#ifndef NATIVEWINDOWBASE_H
#define NATIVEWINDOWBASE_H

#include <android/system/window.h>
#include <EGL/egl.h>

#define NO_ERROR                0L
#define BAD_VALUE               -1

//#define TRACE printf
#define TRACE(...)

/**
 * @brief A Class to do common ANativeBuffer initialization and thunk c-style
 *        callbacks into C++ method calls.
 **/
class BaseNativeWindowBuffer : public ANativeWindowBuffer
{
protected:
	BaseNativeWindowBuffer();

private:
	unsigned int refcount;
	static void _decRef(struct android_native_base_t* base);
	static void _incRef(struct android_native_base_t* base);
};

/**
 * @brief A Class to do common ANativeWindow initialization and thunk c-style
 *        callbacks into C++ method calls.
 **/
class BaseNativeWindow : public ANativeWindow
{
public:
	operator EGLNativeWindowType()
	{
		EGLNativeWindowType ret = reinterpret_cast<EGLNativeWindowType>((ANativeWindow*)this);
		TRACE("casting %p to %p\n", this, ret);
		return ret;
	}

protected:
	BaseNativeWindow();
	~BaseNativeWindow();

	// does this require more magic?
	unsigned int refcount;
	static void _decRef(struct android_native_base_t* base);
	static void _incRef(struct android_native_base_t* base);

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
	static int _setSwapInterval(struct ANativeWindow* window, int interval);
	static int _dequeueBuffer(ANativeWindow* window, ANativeWindowBuffer** buffer);
	static const char *_native_window_operation(int what);
	static const char *_native_query_operation(int what);
	static int _lockBuffer(struct ANativeWindow* window, ANativeWindowBuffer* buffer);
	static int _queueBuffer(struct ANativeWindow* window, ANativeWindowBuffer* buffer);
	static int _query(const struct ANativeWindow* window, int what, int* value);
	static int _perform(struct ANativeWindow* window, int operation, ... );
	static int _cancelBuffer(struct ANativeWindow* window, ANativeWindowBuffer* buffer);
};

#endif
