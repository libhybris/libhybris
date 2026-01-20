glesv2 = library('GLESv2' + gl_lib_suffix,
  'glesv2.c',
  dependencies : hybris_common_dep,
  include_directories: header_inc,
  version : '2.0.0',
  install : true
)

glesv2_dep = declare_dependency(
  link_with : glesv2
)

if not want_glvnd
pkg.generate(glesv2,
  name : 'glesv2',
  description : 'libhybris OpenGL ES 2 library',
  libraries : ['-lGLESv2'],
  version : lib_version
)
endif
