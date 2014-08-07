SUBDIRS = include properties common hardware

if HAS_ANDROID_4_2_0
SUBDIRS += libsync
endif

SUBDIRS += egl glesv1 glesv2 ui sf input camera media wifi
if HAS_LIBNFC_NXP_HEADERS
SUBDIRS += libnfc_nxp libnfc_ndef_nxp
endif
SUBDIRS += utils tests


MAINTAINERCLEANFILES = \
	aclocal.m4 compile config.guess config.sub \
	configure depcomp install-sh ltmain.sh     \
	Makefile.in missing config.h.in            \
	mkinstalldirs *~ \
	$(NULL)
