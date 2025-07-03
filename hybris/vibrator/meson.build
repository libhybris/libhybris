libvibrator = library('vibrator',
  'vibrator.c',
  include_directories: header_inc,
  dependencies : [hybris_dep, hybris_common_dep],
  version : '1.0.0',
  install : true
)

vibrator_dep = declare_dependency(
  link_with : libvibrator
)

pkg.generate(libvibrator,
  filebase : 'libvibrator',
  name : 'hybris-vibrator',
  description : 'libhybris vibrator library',
  libraries : [hybris_common_dep, libvibrator],
  version : lib_version
)
