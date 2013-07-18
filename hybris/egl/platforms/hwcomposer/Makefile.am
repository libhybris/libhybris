pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = hwcomposer-egl.pc

hwcomposerwindowincludedir = $(includedir)/hybris/hwcomposerwindow
hwcomposerwindowinclude_HEADERS = \
        hwcomposer_window.h

pkglib_LTLIBRARIES = eglplatform_hwcomposer.la

eglplatform_hwcomposer_la_SOURCES = \
	eglplatform_hwcomposer.cpp \
	hwcomposer_window.cpp

eglplatform_hwcomposer_la_CXXFLAGS = \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/common \
	-I$(top_srcdir)/egl \
	-I$(top_srcdir)/egl/platforms/common \
	-I$(top_srcdir)/include/android

if WANT_TRACE
eglplatform_hwcomposer_la_CXXFLAGS += -DDEBUG
endif
if WANT_DEBUG
eglplatform_hwcomposer_la_CXXFLAGS += -ggdb -O0
endif

eglplatform_hwcomposer_la_LDFLAGS = \
	-version-info "1":"0":"0" \
	-avoid-version -module -shared -export-dynamic \
	$(top_builddir)/egl/platforms/common/libhybris-eglplatformcommon.la \
	$(top_builddir)/hardware/libhardware.la

if HAS_ANDROID_4_2_0
eglplatform_hwcomposer_la_LDFLAGS += $(top_builddir)/libsync/libsync.la
endif

