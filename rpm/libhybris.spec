Name:      libhybris	
Version:   0.0.0
Release:   1%{?dist}
Summary:   Hybris allowing us to use bionic-based HW adaptations in glibc systems

Group:	   System
License:   Apache 2.0
URL:	   https://github.com/libhybris/libhybris
Source0:   %{name}-%{version}.tar.bz2
BuildRequires: libtool

%description
%{summary}.

%package devel
Summary: Common development headers for libhybris
Group:   Development/Libraries
Requires: %{name} = %{version}-%{release}

%description devel
%{summary}.

%package libEGL
Summary: EGL for hybris
Group: Development/Libraries
Requires: %{name} = %{version}-%{release}
Requires(post): /sbin/ldconfig
Requires(postun): /sbin/ldconfig
Provides: libEGL
Provides: libEGL.so.1

%description libEGL
%{summary}.

%package libEGL-devel
Summary: EGL development headers for hybris
Group: Development/Libraries
Requires: %{name} = %{version}-%{release}
Requires: %{name}-libEGL = %{version}-%{release}
Requires: %{name}-devel = %{version}-%{release}
Provides: libEGL-devel

%description libEGL-devel
%{summary}.

%package libGLESv1
Summary: GLESv1 for hybris
Group: Development/Libraries
Requires: %{name} = %{version}-%{release}
Requires(post): /sbin/ldconfig
Requires(postun): /sbin/ldconfig
Provides: libGLESv1
Provides: libGLES_CM.so.1

%description libGLESv1
%{summary}.

%package libGLESv1-devel
Summary: GLESv1 development headers for hybris
Group: Development/Libraries
Requires: %{name} = %{version}-%{release}
Requires: %{name}-libGLESv1 = %{version}-%{release}
Requires: %{name}-devel = %{version}-%{release}
Provides: libGLESv1-devel

%description libGLESv1-devel
%{summary}.

%package libGLESv2
Summary: GLESv2 for hybris
Group: Development/Libraries
Requires: %{name} = %{version}-%{release}
Requires(post): /sbin/ldconfig
Requires(postun): /sbin/ldconfig
Provides: libGLESv2
Provides: libGLESv2.so.2

%description libGLESv2
%{summary}.

%package libGLESv2-devel
Summary: GLESv2 development headers for hybris
Group: Development/Libraries
Requires: %{name} = %{version}-%{release}
Requires: %{name}-libGLESv2 = %{version}-%{release}
Requires: %{name}-devel = %{version}-%{release}
Provides: libGLESv2-devel

%description libGLESv2-devel
%{summary}.

%package libOpenCL
Summary: OpenCL for hybris
Group: Development/Libraries
Requires: %{name} = %{version}-%{release}
Requires(post): /sbin/ldconfig
Requires(postun): /sbin/ldconfig
Provides: libOpenCL

%description libOpenCL
%{summary}.

%package libOpenCL-devel
Summary: OpenVG development headers for hybris
Group: Development/Libraries
Requires: %{name} = %{version}-%{release}
Requires: %{name}-libOpenCL = %{version}-%{release}
Requires: %{name}-devel = %{version}-%{release}
Provides: libOpenCL-devel

%description libOpenCL-devel
%{summary}.

%package libOpenVG
Summary: OpenVG for hybris
Group: Development/Libraries
Requires: %{name} = %{version}-%{release}
Requires(post): /sbin/ldconfig
Requires(postun): /sbin/ldconfig
Provides: libOpenVG

%description libOpenVG
%{summary}.

%package libOpenVG-devel
Summary: OpenVG development headers for hybris
Group: Development/Libraries
Requires: %{name} = %{version}-%{release}
Requires: %{name}-libOpenVG = %{version}-%{release}
Requires: %{name}-devel = %{version}-%{release}
Provides: libOpenVG-devel

%description libOpenVG-devel
%{summary}.

%package libhardware
Summary: The libhardware wrapping of libhybris
Requires(post): /sbin/ldconfig
Requires(postun): /sbin/ldconfig
Group:   System/Libraries

%description libhardware
%{summary}.

%package libui
Summary: The libui of libhybris
Requires(post): /sbin/ldconfig
Requires(postun): /sbin/ldconfig
Group:   System/Libraries

%description libui
%{summary}.

%package libui-devel
Summary: The development files for libui of libhybris
Requires(post): /sbin/ldconfig
Requires(postun): /sbin/ldconfig
Group:   System/Libraries

%description libui-devel
%{summary}.

%package libhardware-devel
Summary: The development headers of libhardware wrapping of libhybris
Requires: %{name}-devel = %{version}-%{release}
Requires: %{name}-libhardware = %{version}-%{release}
Group: Development/Libraries

%description libhardware-devel
%{summary}.

%package tests
Summary: Tests for %{name}
Group:   System/Libraries

%description tests
%{summary}.

%prep
%setup -q

%build
cd hybris
autoreconf -v -f -i
%configure \
%ifarch %{arm}
--enable-arch=arm \
%endif
%ifarch %{ix86}
--enable-arch=x86 \
%endif
%{nil}

make %{?_smp_mflags}

%install
rm -rf $RPM_BUILD_ROOT
cd hybris
make install DESTDIR=$RPM_BUILD_ROOT

# Remove the static libraries.
rm %{buildroot}/%{_libdir}/*.la %{buildroot}/%{_libdir}/libhybris/*.la

%post -p /sbin/ldconfig
%postun -p /sbin/ldconfig

%post libEGL -p /sbin/ldconfig
%postun libEGL -p /sbin/ldconfig

%post libGLESv2 -p /sbin/ldconfig
%postun libGLESv2 -p /sbin/ldconfig

%post libhardware -p /sbin/ldconfig
%postun libhardware -p /sbin/ldconfig

%post libui -p /sbin/ldconfig
%postun libui -p /sbin/ldconfig

%files
%defattr(-,root,root,-)
%doc hybris/AUTHORS hybris/COPYING
%{_libdir}/libhybris-common.so.*

%files devel
%defattr(-,root,root,-)
%{_includedir}/android/cutils/*.h
%{_includedir}/android/system/*.h
%{_libdir}/libhybris-common.so

%files libEGL
%defattr(-,root,root,-)
%{_libdir}/libEGL.so.*
%{_libdir}/libhybris-eglplatformcommon.so.*
%{_libdir}/libhybris/eglplatform_fbdev.so
%{_libdir}/libhybris/eglplatform_null.so

%files libEGL-devel
%defattr(-,root,root,-)
%{_includedir}/KHR/*.h
%{_includedir}/EGL/*.h
%{_libdir}/libEGL.so
%{_libdir}/libhybris-eglplatformcommon.so

%files libGLESv1
%defattr(-,root,root,-)
# We don't have implementation of GLESv1 atm.

%files libGLESv1-devel
%defattr(-,root,root,-)
%{_includedir}/GLES/*.h

%files libGLESv2
%defattr(-,root,root,-)
%{_libdir}/libGLESv2.so.2*

%files libGLESv2-devel
%defattr(-,root,root,-)
%{_includedir}/GLES2/*.h
%{_libdir}/libGLESv2.so

%files libOpenCL
%defattr(-,root,root,-)
# We don't have implementation of OpenCL atm.

%files libOpenCL-devel
%defattr(-,root,root,-)
%{_includedir}/CL/*.h
%{_includedir}/CL/*.hpp

%files libOpenVG
%defattr(-,root,root,-)
# We don't have implementation of OpenVG atm.

%files libOpenVG-devel
%defattr(-,root,root,-)
%{_includedir}/VG/*.h

%files libhardware
%defattr(-,root,root,-)
%{_libdir}/libhardware.so.*

%files libhardware-devel
%defattr(-,root,root,-)
%{_libdir}/libhardware.so
%{_includedir}/android/hardware/*.h

%files libui
%defattr(-,root,root,-)
%{_libdir}/libui.so.*

%files libui-devel
%defattr(-,root,root,-)
%{_libdir}/libui.so
%{_includedir}/hybris/ui_compatibility_layer.h

%files tests
%defattr(-,root,root,-)
%{_bindir}/test_*

