libcamera_sources = [
  'camera.c',
]

libcamera = library('camera',
  libcamera_sources,
  include_directories: [header_inc],
  dependencies : [hybris_dep, hybris_common_dep],
  version : '1.0.0',
  install : true
)

camera_dep = declare_dependency(link_with:libcamera
)

pkg.generate(libcamera,
  filebase : 'libcamera',
  name : 'hybris-camera',
  description : 'libhybris camera library',
  libraries : [libcamera, hybris_common_dep],
  version : lib_version
)
