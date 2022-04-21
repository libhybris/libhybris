lib_LTLIBRARIES = \
	libhybris-platformcommon.la

libhybris_platformcommon_la_SOURCES = \
	native_handle.c \
	nativewindowbase.cpp \
	platformcommon.cpp \
	windowbuffer.cpp

if WANT_WAYLAND

libhybris_platformcommon_la_SOURCES += \
	server_wlegl.cpp \
	server_wlegl_handle.cpp \
	server_wlegl_buffer.cpp \
	wayland-android-protocol.c

BUILT_SOURCES = wayland-android-protocol.c \
		wayland-android-client-protocol.h \
		wayland-android-server-protocol.h

%-protocol.c : %.xml
	$(AM_V_GEN)$(WAYLAND_SCANNER) code < $< > $@

%-server-protocol.h : %.xml
	$(AM_V_GEN)$(WAYLAND_SCANNER) server-header < $< > $@

%-client-protocol.h : %.xml
	$(AM_V_GEN)$(WAYLAND_SCANNER) client-header < $< > $@

endif

libhybris_platformcommon_la_CFLAGS = -I$(top_srcdir)/include $(ANDROID_HEADERS_CFLAGS) -I$(top_srcdir)/common/
if WANT_WAYLAND
libhybris_platformcommon_la_CFLAGS += $(WAYLAND_CLIENT_CFLAGS) $(WAYLAND_SERVER_CFLAGS) $(ANDROID_HEADERS_CFLAGS)
endif

if WANT_MESA
libhybris_platformcommon_la_CFLAGS += -DLIBHYBRIS_WANTS_MESA_X11_HEADERS
endif
if WANT_TRACE
libhybris_platformcommon_la_CFLAGS += -DDEBUG
endif
if WANT_DEBUG
libhybris_platformcommon_la_CFLAGS += -ggdb -O0
endif


libhybris_platformcommon_la_CXXFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/egl $(ANDROID_HEADERS_CFLAGS) -I$(top_srcdir)/common/
if WANT_MESA
libhybris_platformcommon_la_CXXFLAGS += -DLIBHYBRIS_WANTS_MESA_X11_HEADERS
endif

if WANT_WAYLAND
libhybris_platformcommon_la_CXXFLAGS += $(WAYLAND_CLIENT_CFLAGS) $(WAYLAND_SERVER_CFLAGS) $(ANDROID_HEADERS_CFLAGS)
endif

if WANT_TRACE
libhybris_platformcommon_la_CXXFLAGS += -DDEBUG
endif
if WANT_DEBUG
libhybris_platformcommon_la_CXXFLAGS += -ggdb -O0
endif
libhybris_platformcommon_la_LDFLAGS = \
	$(top_builddir)/common/libhybris-common.la \
	-version-info "1":"0":"0"

if HAS_ANDROID_4_2_0
libhybris_platformcommon_la_LDFLAGS += $(top_builddir)/libsync/libsync.la
endif

if HAS_ANDROID_5_0_0
libhybris_platformcommon_la_LDFLAGS += $(top_builddir)/libsync/libsync.la
endif


platformcommondir = $(includedir)/hybris/platformcommon
platformcommon_HEADERS = \
	support.h \
	nativewindowbase.h

if WANT_WAYLAND
libhybris_platformcommon_la_LDFLAGS += \
	$(WAYLAND_CLIENT_LIBS) \
	$(WAYLAND_SERVER_LIBS)
endif
