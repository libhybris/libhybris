libhybris_vulkanplatformcommon_sources = [
  'vulkanplatformcommon.cpp',
]

libhybris_vulkanplatformcommon_deps = [
  hybris_platformcommon_dep,
]

if (android_headers_major >= 4 and android_headers_minor >= 2) or \
   android_headers_major >= 5
  libhybris_vulkanplatformcommon_deps += [sync_dep]
endif

if want_wayland
  libhybris_vulkanplatformcommon_deps += [wayland_client_dep, wayland_egl_dep]
endif

libhybris_vulkanplatformcommon = library('hybris-vulkanplatformcommon',
  libhybris_vulkanplatformcommon_sources,
  include_directories: [config_inc, header_inc, vulkan_inc],
  dependencies : libhybris_vulkanplatformcommon_deps,
  version : '1.0.0',
  install : true
)

hybris_vulkanplatformcommon_dep = declare_dependency(
  link_with : libhybris_vulkanplatformcommon,
  include_directories : ['.', config_inc, header_inc, vulkan_inc],
  dependencies : libhybris_vulkanplatformcommon_deps,
)
