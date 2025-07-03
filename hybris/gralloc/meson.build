libgralloc_sources = [
  'gralloc.c',
]

if cc.has_header('hardware/gralloc1.h', dependencies : [hybris_dep])
  libgralloc_sources += ['GrallocUsageConversion.cpp']
endif

deps = [hardware_dep, hybris_dep, hybris_common_dep, ui_dep]

libgralloc = library('gralloc',
  libgralloc_sources,
  include_directories: [header_inc],
  dependencies : deps,
  version : '1.0.0',
  install : true
)

gralloc_dep = declare_dependency(
  link_with : libgralloc,
  include_directories: ['.'],
)

pkg.generate(libgralloc,
  filebase : 'libgralloc',
  name : 'hybris-gralloc',
  description : 'libhybris gralloc library',
  libraries : libgralloc,
  requires : android_headers_pkgconfig,
  version : lib_version
)
