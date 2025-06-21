libhybris_vulkanplatform_waylandwindow_sources = [
  '../../../platforms/wayland/wayland_window_common.cpp',
  'wayland_window.cpp',
]

libhybris_vulkanplatform_waylandwindow_deps = [
  hybris_vulkanplatformcommon_dep,
  hybris_platformwayland_dep,
  wayland_client_dep,
]

if hybris_no_server_side_buffers
  libhybris_vulkanplatform_waylandwindow_cpp_args = ['-DHYBRIS_NO_SERVER_SIDE_BUFFERS']
else
  libhybris_vulkanplatform_waylandwindow_cpp_args = []
endif

libhybris_vulkanplatform_waylandwindow = library('hybris-waylandwindow',
  libhybris_vulkanplatform_waylandwindow_sources,
  cpp_args : libhybris_vulkanplatform_waylandwindow_cpp_args,
  dependencies : libhybris_vulkanplatform_waylandwindow_deps
)

hybris_vulkanplatform_waylandwindow_dep = declare_dependency(
  link_with : libhybris_vulkanplatform_waylandwindow,
  dependencies : libhybris_vulkanplatform_waylandwindow_deps,
)

vulkanplatform_wayland_sources = [
  'vulkanplatform_wayland.cpp',
]

vulkanplatform_wayland_deps = [
  dl_dep,
  hybris_vulkanplatform_waylandwindow_dep,
]

vulkanplatform_wayland = library('vulkanplatform_wayland',
  vulkanplatform_wayland_sources,
  dependencies : vulkanplatform_wayland_deps,
  name_prefix : '',
  install_dir : get_option('libdir') / 'libhybris',
  install : true
)
