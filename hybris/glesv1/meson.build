glesv1_cm = library('GLESv1_CM' + gl_lib_suffix,
  'glesv1_cm.c',
  dependencies : hybris_common_dep,
  include_directories: header_inc,
  version : '1.0.1',
  install : true
)

glesv1_dep = declare_dependency(
  link_with : glesv1_cm
)

if not want_glvnd
pkg.generate(glesv1_cm,
  name : 'glesv1_cm',
  description : 'libhybris OpenGL ES 1.1 library',
  libraries : ['-lGLESv1_CM'],
  version : lib_version
)
endif
