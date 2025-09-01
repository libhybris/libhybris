egl_inc = include_directories('.')

subdir('platforms')

libegl_sources = [
  'egl.c',
  'helper.cpp',
  'ws.c',
]

egl_headers = [
  'eglhybris.h',
]

ws_headers = [
  'ws.h',
]

egl_plugin_install_dir = join_paths(get_option('libdir'), 'libhybris')

egl_c_args = [
  '-DPKGLIBDIR="@0@"'.format(egl_plugin_install_dir),
  '-DDEFAULT_EGL_PLATFORM="@0@"'.format(get_option('default_egl_platform'))
]

egl_include_directories = [config_inc, header_inc]

egl_deps = [dl_dep, hardware_dep, hybris_dep, hybris_common_dep, hybris_platformcommon_dep]

if want_glvnd
  egl_c_args += ['-DGL_LIB_SUFFIX="@0@"'.format(gl_lib_suffix)]
  egl_include_directories += include_directories('.', 'glvnd')
  subdir('glvnd')
  egl_deps += [glvnd_dep, ext_egl_dep, ext_glesv2_dep]

  libegl_sources += [
    'glvnd/eglglvnd.cpp',
    'glvnd/egldispatchstubs.c',
    'glvnd/egldispatchstubs.h',
    g_egldispatchstubs_c,
    g_egldispatchstubs_h,
  ]
  egl_c_args += ['-DWANT_GLVND=1']

else
  egl_c_args += ['-DGL_LIB_SUFFIX=""']
endif

install_headers(egl_headers, subdir : 'EGL')
install_headers(ws_headers, subdir : 'hybris/eglplatformcommon')

egl = library('EGL' + gl_lib_suffix,
  libegl_sources,
  include_directories : egl_include_directories,
  c_args : egl_c_args,
  dependencies : egl_deps,
  version : '1.0.0',
  install : true
)

egl_dep = declare_dependency(
  link_with : egl,
  include_directories: ['.'],
)

if not want_glvnd
  pkg.generate(egl,
    name : 'egl',
    description : 'libhybris EGL library',
    libraries : [egl],
    version : '21.1.6'
  )
endif
