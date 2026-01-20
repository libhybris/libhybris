libsf = library('sf',
  'sf.c',
  include_directories: header_inc,
  dependencies : [hybris_dep, hybris_common_dep],
  version : '1.0.0',
  install : true
)

sf_dep = declare_dependency(
  link_with : libsf
)

pkg.generate(libsf,
  filebase : 'libsf',
  name : 'hybris-surface-flinger',
  description : 'libhybris surface flinger library',
  libraries : [hybris_common_dep, libsf],
  version : lib_version
)
