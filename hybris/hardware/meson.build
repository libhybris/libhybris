libhardware = library('hardware',
  'hardware.c',
  dependencies : [hybris_dep, hybris_common_dep],
  include_directories: header_inc,
  version : '2.0.0',
  install : true
)

hardware_dep = declare_dependency(
  link_with : libhardware
)

pkg.generate(libhardware,
  filebase : 'libhardware',
  name : 'hybris-hardware',
  description : 'libhybris hardware library',
  libraries : libhardware,
  requires : android_headers_pkgconfig,
  version : lib_version
)
