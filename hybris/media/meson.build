libmedia = library('media',
  'media.c',
  include_directories: header_inc,
  dependencies : [hybris_dep, hybris_common_dep],
  version : '1.0.0',
  install : true
)

media_dep = declare_dependency(
  link_with : libmedia
)

pkg.generate(libmedia,
  filebase : 'libmedia',
  name : 'hybris-media',
  description : 'libhybris media library',
  libraries : [hybris_common_dep, libmedia],
  version : lib_version
)
