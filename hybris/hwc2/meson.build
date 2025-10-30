if has_hwcomposer2_headers
  libhwc2_c_args = ['-DHAS_HWCOMPOSER2_HEADERS=1']
else
  libhwc2_c_args = []
endif

libhwc2 = library('hwc2',
  'hwc2.c',
  c_args : libhwc2_c_args,
  dependencies : [hybris_dep, hybris_common_dep],
  include_directories: header_inc,
  version : '1.0.0',
  install : true
)

hwc2_dep = declare_dependency(
  link_with : libhwc2,
  compile_args : libhwc2_c_args,
)

pkg.generate(libhwc2,
  filebase : 'libhwc2',
  name : 'hybris-hwcomposer2',
  description : 'libhybris hwcomposer2 library',
  libraries : [hybris_common_dep, libhwc2],
  requires : android_headers_pkgconfig,
  version : lib_version
)
