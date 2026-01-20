jb_sources = [
  'dlfcn.c',
  'linker.c',
  'linker_environ.c',
  'linker_format.c',
  'rt.c',
  '../strlcpy.c',
]

if get_option('arch') == 'arm'
  jb_c_args = [
    '-DANDROID_ARM_LINKER',
    '-DHAVE_ARM_TLS_REGISTER'
  ]
endif

if get_option('arch') == 'x86'
  jb_c_args = ['-DANDROID_X86_LINKER']
endif

if get_option('enable_debug')
  jb_c_args += ['-DLINKER_DEBUG=1']
else
  jb_c_args += ['-DLINKER_DEBUG=0']
endif

jb = both_libraries('jb',
  jb_sources,
  include_directories : [header_inc, config_inc, common_inc],
  c_args : jb_c_args,
  dependencies : hybris_dep,
  name_prefix : '',
  install_dir : get_option('libdir') / 'libhybris/linker',
  install : true
)
