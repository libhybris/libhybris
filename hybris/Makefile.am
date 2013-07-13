SUBDIRS = include common hardware

if HAS_ANDROID_4_2_0
SUBDIRS += libsync
endif

SUBDIRS += egl glesv1 glesv2 ui sf input camera tests


MAINTAINERCLEANFILES = \
	aclocal.m4 compile config.guess config.sub \
	configure depcomp install-sh ltmain.sh     \
	Makefile.in missing config.h.in            \
	mkinstalldirs *~ \
	$(NULL)
