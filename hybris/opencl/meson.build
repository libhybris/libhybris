libopencl = library('OpenCL',
  'opencl.c',
  include_directories: header_inc,
  dependencies : [hybris_dep, hybris_common_dep],
  version : '1.0.0',
  install : true
)

opencl_dep = declare_dependency(
  link_with : libopencl
)

pkg.generate(libopencl,
  filebase : 'OpenCL',
  name : 'opencl',
  description : 'libhybris OpenCL library',
  libraries : [hybris_common_dep, libopencl],
  version : '1.0'
)
