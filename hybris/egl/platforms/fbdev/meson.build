eglplatform_fbdev_sources = [
  'eglplatform_fbdev.cpp',
  'fbdev_window.cpp',
]

eglplatform_fbdev = library('eglplatform_fbdev',
  eglplatform_fbdev_sources,
  dependencies : hybris_eglplatformcommon_dep,
  name_prefix : '',
  install_dir : get_option('libdir') / 'libhybris',
  install : true
)
