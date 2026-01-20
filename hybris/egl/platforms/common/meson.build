libhybris_eglplatformcommon_sources = [
  'eglplatformcommon.cpp',
]

libhybris_eglplatformcommon_headers = [
  'eglplatformcommon.h',
  'eglnativewindowbase.h',
  'hybris_nativebufferext.h',
]

libhybris_eglplatformcommon_deps = [
  hybris_platformcommon_dep,
]

if (android_headers_major >= 4 and android_headers_minor >= 2) or \
   android_headers_major >= 5
  libhybris_eglplatformcommon_deps += [sync_dep]
endif

if want_wayland
  libhybris_eglplatformcommon_deps += [
    wayland_client_dep,
    wayland_egl_dep,
    wayland_server_dep
  ]
endif

install_headers(libhybris_eglplatformcommon_headers, subdir : 'hybris/eglplatformcommon')

libhybris_eglplatformcommon = library('hybris-eglplatformcommon',
  libhybris_eglplatformcommon_sources,
  include_directories : [header_inc, config_inc, egl_inc],
  dependencies : libhybris_eglplatformcommon_deps,
  version : '1.0.0',
  install : true
)

hybris_eglplatformcommon_dep = declare_dependency(
  link_with : libhybris_eglplatformcommon,
  include_directories : ['.', config_inc, header_inc, egl_inc],
  dependencies : libhybris_eglplatformcommon_deps,
)

libhybris_eglplatformcommon_headers = [
  'eglplatformcommon.h',
  'eglnativewindowbase.h',
  'hybris_nativebufferext.h',
]
install_headers(libhybris_eglplatformcommon_headers, subdir : 'hybris/common')

pkg.generate(libhybris_eglplatformcommon,
  name : 'hybris-egl-platform',
  description : 'libhybris EGL platform library',
  libraries : [hybris_dep, hybris_common_dep, hybris_eglplatformcommon_dep, hybris_platformcommon_dep],
  requires : android_headers_pkgconfig,
  subdirs : ['hybris/eglplatformcommon', 'hybris/platformcommon'],
  version : lib_version
)
