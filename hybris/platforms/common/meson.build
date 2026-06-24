libhybris_platformcommon_sources = [
  'nativewindowbase.cpp',
  'platformcommon.cpp',
  'windowbuffer.cpp',
]

libhybris_platformcommon_headers = [
  'nativewindowbase.h',
  'support.h',
]

libhybris_platformcommon_deps = [
  hybris_dep,
  hybris_common_dep,
  gralloc_dep
]

if (android_headers_major >= 4 and android_headers_minor >= 2) or \
   android_headers_major >= 5
  libhybris_platformcommon_deps += [sync_dep]
endif

if want_wayland
  prog_wayland_scanner = find_program('wayland-scanner')

  xml_source = 'wayland-android'

  wayland_android_protocol_c = custom_target(
    xml_source + 'protocol.c',
    output : xml_source + '-protocol.c',
    input : xml_source + '.xml',
    command : [prog_wayland_scanner, 'code', '@INPUT@', '@OUTPUT@'],
  )

  wayland_android_server_protocol_h = custom_target(
    xml_source + 'server-protocol.h',
    output : xml_source + '-server-protocol.h',
    input : xml_source + '.xml',
    command : [prog_wayland_scanner, 'server-header', '@INPUT@', '@OUTPUT@'],
  )

  wayland_android_client_protocol_h = custom_target(
    xml_source + 'client-protocol.h',
    output : xml_source + '-client-protocol.h',
    input : xml_source + '.xml',
    command : [prog_wayland_scanner, 'client-header', '@INPUT@', '@OUTPUT@'],
  )

  libhybris_platformcommon_sources += [
    'server_wlegl.cpp',
    'server_wlegl_handle.cpp',
    'server_wlegl_buffer.cpp',
    wayland_android_protocol_c,
    wayland_android_server_protocol_h,
    wayland_android_client_protocol_h,
  ]
  libhybris_platformcommon_deps += [
    wayland_client_dep,
    wayland_egl_dep,
    wayland_server_dep
  ]
endif

install_headers(libhybris_platformcommon_headers, subdir : 'hybris/platformcommon')

libhybris_platformcommon = library('hybris-platformcommon',
  libhybris_platformcommon_sources,
  include_directories: [config_inc, header_inc],
  dependencies : libhybris_platformcommon_deps,
  version : '1.0.0',
  install : true
)

hybris_platformcommon_dep = declare_dependency(
  link_with : libhybris_platformcommon,
  include_directories: ['.', config_inc, header_inc],
  dependencies : libhybris_platformcommon_deps,
)
