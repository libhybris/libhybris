libhybris_eglplatform_waylandwindow_sources = [
  '../../../platforms/wayland/wayland_window_common.cpp',
  'wayland_window.cpp',
]

libhybris_eglplatform_waylandwindow_deps = [
  hybris_eglplatformcommon_dep,
  hybris_platformwayland_dep,
  wayland_client_dep,
]

if hybris_no_server_side_buffers
  libhybris_eglplatform_waylandwindow_cpp_args = ['-DHYBRIS_NO_SERVER_SIDE_BUFFERS']
else
  libhybris_eglplatform_waylandwindow_cpp_args = []
endif

libhybris_eglplatform_waylandwindow = library('hybris-waylandwindow',
  libhybris_eglplatform_waylandwindow_sources,
  cpp_args : libhybris_eglplatform_waylandwindow_cpp_args,
  dependencies : libhybris_eglplatform_waylandwindow_deps
)

hybris_waylandwindow_dep = declare_dependency(
  link_with : libhybris_eglplatform_waylandwindow,
  dependencies : libhybris_eglplatform_waylandwindow_deps,
)

eglplatform_wayland_sources = [
  'eglplatform_wayland.cpp',
]

eglplatform_wayland_deps = [
  dl_dep,
  hybris_waylandwindow_dep,
]

eglplatform_wayland = library('eglplatform_wayland',
  eglplatform_wayland_sources,
  dependencies : eglplatform_wayland_deps,
  name_prefix : '',
  install_dir : get_option('libdir') / 'libhybris',
  install : true
)
