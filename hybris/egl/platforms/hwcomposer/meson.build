libhybris_hwcomposerwindow_deps = [
  hybris_eglplatformcommon_dep
]

libhybris_hwcomposerwindow = library('hybris-hwcomposerwindow',
  'hwcomposer_window.cpp',
  dependencies : libhybris_hwcomposerwindow_deps,
  version : '1.0.0',
  install : true
)

hybris_hwcomposerwindow_dep = declare_dependency(
  link_with : libhybris_hwcomposerwindow,
  include_directories: ['.'],
  dependencies : libhybris_hwcomposerwindow_deps,
)

hybris_hwcomposerwindow_headers = [
  'hwcomposer.h',
  'hwcomposer_window.h',
]
install_headers(hybris_hwcomposerwindow_headers, subdir : 'hybris/hwcomposerwindow')

pkg.generate(libhybris_hwcomposerwindow,
  name : 'hwcomposer-egl',
  description : 'libhybris EGL platform library',
  libraries : [libhybris_hwcomposerwindow],
  subdirs : 'hybris/hwcomposerwindow',
  version : lib_version
)

eglplatform_hwcomposer = library('eglplatform_hwcomposer',
  'eglplatform_hwcomposer.cpp',
  dependencies : hybris_hwcomposerwindow_dep,
  name_prefix : '',
  install_dir : get_option('libdir') / 'libhybris',
  install : true
)
