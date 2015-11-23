bin_PROGRAMS = \
	test_egl \
	test_egl_configs \
	test_glesv2 \
	test_sensors \
	test_vibrator \
	test_gps

if HAS_ANDROID_4_2_0
bin_PROGRAMS += test_hwcomposer
endif

if HAS_ANDROID_5_0_0
bin_PROGRAMS += test_hwcomposer
endif


if HAS_LIBNFC_NXP_HEADERS
# test_nfc depends on NFC hardware HAL interface, which is only
# available until Android API level 15 (v4.0.3, v4.0.4).
if HAS_ANDROID_4_0_3
bin_PROGRAMS += test_nfc
else
if HAS_ANDROID_4_1_0
bin_PROGRAMS += test_nfc
else
if HAS_ANDROID_5_0_0
bin_PROGRAMS += test_nfc
endif
endif
endif
endif

bin_PROGRAMS += test_audio
test_audio_SOURCES = test_audio.c
test_audio_CFLAGS = \
	-I$(top_srcdir)/include
test_audio_LDADD = \
	$(top_builddir)/common/libhybris-common.la \
	$(top_builddir)/hardware/libhardware.la

test_egl_SOURCES = test_egl.c
test_egl_CFLAGS = \
	-I$(top_srcdir)/include \
	$(ANDROID_HEADERS_CFLAGS) \
	-I$(top_srcdir)/egl/platforms/common

if WANT_MESA
test_egl_CFLAGS += -DLIBHYBRIS_WANTS_MESA_X11_HEADERS
endif
test_egl_LDADD = \
	$(top_builddir)/common/libhybris-common.la \
	$(top_builddir)/egl/libEGL.la

test_egl_configs_SOURCES = test_egl_configs.c
test_egl_configs_CFLAGS = \
	-I$(top_srcdir)/include \
	$(ANDROID_HEADERS_CFLAGS)
test_egl_configs_LDADD = \
	$(top_builddir)/common/libhybris-common.la \
	$(top_builddir)/egl/libEGL.la

test_glesv2_SOURCES = test_glesv2.c
test_glesv2_CFLAGS = \
	-I$(top_srcdir)/include \
	$(ANDROID_HEADERS_CFLAGS)
if WANT_MESA
test_glesv2_CFLAGS += -DLIBHYBRIS_WANTS_MESA_X11_HEADERS
endif
test_glesv2_LDADD = \
	-lm \
	$(top_builddir)/common/libhybris-common.la \
	$(top_builddir)/egl/libEGL.la \
	$(top_builddir)/glesv2/libGLESv2.la

test_hwcomposer_SOURCES = test_hwcomposer.cpp
test_hwcomposer_CXXFLAGS = \
	-I$(top_srcdir)/include \
	$(ANDROID_HEADERS_CFLAGS) \
	-I$(top_srcdir)/common \
	-I$(top_srcdir)/egl/platforms/common \
	-I$(top_srcdir)/egl/platforms/hwcomposer \
	-I$(top_srcdir)/libsync

if WANT_MESA
test_hwcomposer_CXXFLAGS += -DLIBHYBRIS_WANTS_MESA_X11_HEADERS
endif
test_hwcomposer_LDADD = \
	-lm \
	$(top_builddir)/common/libhybris-common.la \
	$(top_builddir)/egl/platforms/hwcomposer/libhybris-hwcomposerwindow.la \
	$(top_builddir)/egl/platforms/common/libhybris-eglplatformcommon.la \
	$(top_builddir)/egl/libEGL.la \
	$(top_builddir)/glesv2/libGLESv2.la \
	$(top_builddir)/libsync/libsync.la \
	$(top_builddir)/hardware/libhardware.la

test_sensors_SOURCES = test_sensors.c
test_sensors_CFLAGS = \
	-I$(top_srcdir)/include \
	$(ANDROID_HEADERS_CFLAGS)
test_sensors_LDADD = \
	$(top_builddir)/common/libhybris-common.la \
	$(top_builddir)/hardware/libhardware.la

test_lights_SOURCES = test_lights.c
test_lights_CFLAGS = \
	-I$(top_srcdir)/include \
	$(ANDROID_HEADERS_CFLAGS)

test_lights_LDADD = \
	$(top_builddir)/common/libhybris-common.la \
	$(top_builddir)/hardware/libhardware.la

test_gps_SOURCES = test_gps.c
test_gps_CFLAGS = -pthread \
	-I$(top_srcdir)/include \
	$(ANDROID_HEADERS_CFLAGS)
test_gps_LDFLAGS = -pthread
test_gps_LDADD =  \
	$(top_builddir)/common/libhybris-common.la \
	$(top_builddir)/hardware/libhardware.la

test_nfc_SOURCES = test_nfc.c
test_nfc_CFLAGS = \
	-I$(top_srcdir)/include \
	$(ANDROID_HEADERS_CFLAGS)

test_nfc_LDFLAGS =
test_nfc_LDADD = \
	$(top_builddir)/libnfc_nxp/libnfc_nxp.la \
	$(top_builddir)/hardware/libhardware.la

test_vibrator_SOURCES = test_vibrator.c
test_vibrator_CFLAGS = \
	-I$(top_srcdir)/include \
	$(ANDROID_HEADERS_CFLAGS)
test_vibrator_LDADD = \
	$(top_builddir)/common/libhybris-common.la \
	$(top_builddir)/vibrator/libvibrator.la

