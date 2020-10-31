SUBDIRS = platforms

lib_LTLIBRARIES = \
	libEGL$(GL_LIB_SUFFIX).la

libEGL__GL_LIB_SUFFIX__la_SOURCES = \
	egl.c \
	helper.cpp \
	ws.c

if WANT_GLVND
libEGL__GL_LIB_SUFFIX__la_SOURCES += \
	glvnd/eglglvnd.cpp \
	glvnd/egldispatchstubs.h \
	glvnd/egldispatchstubs.c

nodist_libEGL__GL_LIB_SUFFIX__la_SOURCES = \
	glvnd/g_egldispatchstubs.h \
	glvnd/g_egldispatchstubs.c

BUILT_SOURCES = \
	glvnd/g_egldispatchstubs.h \
	glvnd/g_egldispatchstubs.c

CLEANFILES = \
	glvnd/g_egldispatchstubs.h \
	glvnd/g_egldispatchstubs.c

DISPATCH_STUB_PY_FILES = \
	$(srcdir)/glvnd/generate/static_data.py \
	$(srcdir)/glvnd/generate/genCommon.py \
	$(srcdir)/glvnd/generate/eglFunctionList.py \
	$(srcdir)/glvnd/generate/gen_egl_dispatch.py

DISPATCH_STUB_XML_FILES = \
	$(srcdir)/glvnd/generate/egl.xml \
	$(srcdir)/glvnd/generate/egl_other.xml \
	$(srcdir)/glvnd/generate/egl_hybris.xml

glvnd/g_egldispatchstubs.h: $(DISPATCH_STUB_PY_FILES) $(DISPATCH_STUB_XML_FILES)
	$(PYTHON) $(srcdir)/glvnd/generate/gen_egl_dispatch.py header \
		$(DISPATCH_STUB_XML_FILES) \
		> $@

glvnd/g_egldispatchstubs.c: $(DISPATCH_STUB_PY_FILES) $(DISPATCH_STUB_XML_FILES)
	$(PYTHON) $(srcdir)/glvnd/generate/gen_egl_dispatch.py source \
		$(DISPATCH_STUB_XML_FILES) \
		> $@

glvnd_icddir = $(datarootdir)/glvnd/egl_vendor.d
glvnd_icd_DATA = glvnd/10_libhybris.json
endif

if !WANT_GLVND
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = egl.pc
endif

libEGL__GL_LIB_SUFFIX__la_CFLAGS = -I$(top_srcdir)/include $(ANDROID_HEADERS_CFLAGS) -DPKGLIBDIR="\"$(pkglibdir)/\"" -I$(top_srcdir)/common -DDEFAULT_EGL_PLATFORM="\"@DEFAULT_EGL_PLATFORM@\""
if WANT_MESA
libEGL__GL_LIB_SUFFIX__la_CFLAGS += -DLIBHYBRIS_WANTS_MESA_X11_HEADERS
endif
if WANT_TRACE
libEGL__GL_LIB_SUFFIX__la_CFLAGS += -DDEBUG
endif
if WANT_DEBUG
libEGL__GL_LIB_SUFFIX__la_CFLAGS += -ggdb -O0
endif
if WANT_GLVND
libEGL__GL_LIB_SUFFIX__la_CFLAGS += $(GLVND_CFLAGS) -I$(srcdir)/glvnd -I$(builddir)/glvnd -fvisibility=hidden
endif

libEGL__GL_LIB_SUFFIX__la_CXXFLAGS = -I$(top_srcdir)/include $(ANDROID_HEADERS_CFLAGS) -I$(top_srcdir)/common -DPKGLIBDIR="\"$(pkglibdir)/\""
if WANT_MESA
libEGL__GL_LIB_SUFFIX__la_CXXFLAGS += -DLIBHYBRIS_WANTS_MESA_X11_HEADERS
endif
if WANT_TRACE
libEGL__GL_LIB_SUFFIX__la_CXXFLAGS += -DDEBUG
endif
if WANT_DEBUG
libEGL__GL_LIB_SUFFIX__la_CXXFLAGS += -ggdb -O0
endif
if WANT_GLVND
libEGL__GL_LIB_SUFFIX__la_CXXFLAGS += $(GLVND_CFLAGS) -I$(srcdir)/glvnd -I$(builddir)/glvnd -fvisibility=hidden
endif

libEGL__GL_LIB_SUFFIX__la_LDFLAGS = \
	-ldl \
	$(top_builddir)/common/libhybris-common.la \
	$(top_builddir)/hardware/libhardware.la
if !WANT_GLVND
libEGL__GL_LIB_SUFFIX__la_LDFLAGS += -version-info "1":"0":"0"
else
libEGL__GL_LIB_SUFFIX__la_LDFLAGS += -version-info "0":"0":"0"
endif

eglincludedir = $(includedir)/EGL
eglinclude_HEADERS = \
	eglhybris.h

wsincludedir = $(includedir)/hybris/eglplatformcommon
wsinclude_HEADERS = \
	ws.h
