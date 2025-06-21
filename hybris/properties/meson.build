libandroid_properties = library('android-properties',
  'hybris_properties.c',
  include_directories: header_inc,
  dependencies : [hybris_dep, hybris_common_dep],
  version : '1.0.0',
  install : true
)

android_properties_dep = declare_dependency(
  link_with : libandroid_properties
)

pkg.generate(libandroid_properties,
  filebase : 'libandroid-properties',
  description : 'android properties library',
  libraries : [libandroid_properties],
  version : lib_version
)
