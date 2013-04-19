pkglib_LTLIBRARIES = eglplatform_fbdev.la

eglplatform_fbdev_la_SOURCES = \
	eglplatform_fbdev.cpp \
	fbdev_window.cpp

eglplatform_fbdev_la_CXXFLAGS = \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/egl \
	-I$(top_srcdir)/egl/platforms/common

eglplatform_fbdev_la_LDFLAGS = \
	-avoid-version -module -shared -export-dynamic \
	$(top_builddir)/egl/platforms/common/libhybris-eglplatformcommon.la \
	$(top_builddir)/hardware/libhardware.la
