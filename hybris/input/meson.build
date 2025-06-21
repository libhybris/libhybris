libis = library('is',
  'is.c',
  dependencies : [hybris_dep, hybris_common_dep],
  include_directories: [header_inc],
  version : '1.0.0',
  install : true
)

is_dep = declare_dependency(
  link_with : libis
)

pkg.generate(libis,
  filebase : 'libis',
  name : 'hybris-input',
  description : 'libhybris input library',
  libraries : [hybris_common_dep, libis],
  version : lib_version
)
