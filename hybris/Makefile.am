SUBDIRS = include properties common hardware

if HAS_ANDROID_LIBSYNC
SUBDIRS += libsync
endif
SUBDIRS += egl glesv1 glesv2 vibrator

if HAS_ANDROID_LIBNFC_NXP
SUBDIRS += libnfc_nxp libnfc_ndef_nxp
endif
SUBDIRS += utils tests


MAINTAINERCLEANFILES = \
	aclocal.m4 compile config.guess config.sub \
	configure depcomp install-sh ltmain.sh     \
	Makefile.in missing config.h.in            \
	mkinstalldirs *~ \
	$(NULL)
