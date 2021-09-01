SUBDIRS = include common properties hardware ui gralloc

if HAS_ANDROID_4_2_0
SUBDIRS += libsync
endif
if HAS_ANDROID_5_0_0
SUBDIRS += libsync
endif
SUBDIRS += platforms egl glesv1 glesv2 sf input camera vibrator media opencl wifi hwc2

if HAS_VULKAN_HEADERS
SUBDIRS += vulkan
endif

SUBDIRS += libnfc_nxp libnfc_ndef_nxp

SUBDIRS += utils tests

MAINTAINERCLEANFILES = \
	aclocal.m4 compile config.guess config.sub \
	configure depcomp install-sh ltmain.sh     \
	Makefile.in missing config.h.in            \
	mkinstalldirs *~ \
	$(NULL)
