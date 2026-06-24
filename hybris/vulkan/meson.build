vulkan_inc = include_directories('.')

subdir('platforms')

libvulkan_sources = [
  'vulkan.c',
  'ws.c',
]

vulkan_plugin_install_dir = join_paths(get_option('libdir'), 'libhybris')

libvulkan = library('vulkan',
  libvulkan_sources,
  include_directories: [config_inc, header_inc],
  c_args : '-DPKGLIBDIR="@0@"'.format(vulkan_plugin_install_dir),
  dependencies : [dl_dep, hybris_dep, hybris_common_dep],
  version : '1.2.183',
  install : true
)

vulkan_dep = declare_dependency(
  link_with : libvulkan,
)
