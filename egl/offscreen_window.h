#ifndef OFFSCREEN_WINDOW_H
#define OFFSCREEN_WINDOW_H

#include <linux/fb.h>
#include <android/hardware/gralloc.h>

#include "nativewindowbase.h"

#define NUM_BUFFERS 3

class OffscreenNativeWindowBuffer : public BaseNativeWindowBuffer
{
	friend class OffscreenNativeWindow;

protected:
	OffscreenNativeWindowBuffer(unsigned int width, unsigned int height,
								unsigned int format, unsigned int usage);
	void* vaddr;

public:
	OffscreenNativeWindowBuffer();

	int writeToFd(int fd);
	int readFromFd(int fd);

	buffer_handle_t getHandle();

	bool locked() { return _locked; }
	void lock() { _locked = true; }
	void unlock() { _locked = false; }

private:
	bool _locked;
};

class OffscreenNativeWindow : public BaseNativeWindow
{
public:
	OffscreenNativeWindow(unsigned int width, unsigned int height, unsigned int format = 5);
	~OffscreenNativeWindow();
	OffscreenNativeWindowBuffer* getFrontBuffer();

	virtual void postBuffer(OffscreenNativeWindowBuffer *buffer) { }
	virtual void waitForBuffer(OffscreenNativeWindowBuffer *buffer) { }

	void resize(unsigned int width, unsigned int height);

protected:
	// overloads from BaseNativeWindow
	virtual int setSwapInterval(int interval);
	virtual int dequeueBuffer(BaseNativeWindowBuffer **buffer);
	virtual int lockBuffer(BaseNativeWindowBuffer* buffer);
	virtual int queueBuffer(BaseNativeWindowBuffer* buffer);
	virtual int cancelBuffer(BaseNativeWindowBuffer* buffer);
	virtual unsigned int type() const;
	virtual unsigned int width() const;
	virtual unsigned int height() const;
	virtual unsigned int format() const;
	virtual unsigned int defaultWidth() const;
	virtual unsigned int defaultHeight() const;
	virtual unsigned int queueLength() const;
	virtual unsigned int transformHint() const;
	// perform calls
	virtual int setUsage(int usage);
	virtual int setBuffersFormat(int format);
	virtual int setBuffersDimensions(int width, int height);
private:
	unsigned int m_frontbuffer;
	unsigned int m_tailbuffer;
	unsigned int m_width;
	unsigned int m_height;
	unsigned int m_format;
	unsigned int m_defaultWidth;
	unsigned int m_defaultHeight;
	unsigned int m_usage;
	OffscreenNativeWindowBuffer* m_buffers[NUM_BUFFERS];
	alloc_device_t* m_alloc;
	const gralloc_module_t* m_gralloc;
private:
	OffscreenNativeWindowBuffer* allocateBuffer();
	void resizeBuffer(int id, OffscreenNativeWindowBuffer *buffer, unsigned int width,
				unsigned int height);

};

#endif
