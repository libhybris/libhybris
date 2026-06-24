libwifi = library('wifi',
  'wifi.c',
  include_directories: header_inc,
  dependencies : [hybris_dep, hybris_common_dep],
  version : '1.0.0',
  install : true
)

wifi_dep = declare_dependency(
  link_with : libwifi
)

pkg.generate(libwifi,
  filebase : 'libwifi',
  name : 'hybris-wifi',
  description : 'libhybris wifi library',
  libraries : [hybris_common_dep, libwifi],
  version : lib_version
)
