libsync = library('sync',
  'sync.c',
  dependencies : [hybris_dep, hybris_common_dep],
  version : '2.0.0',
  install : true
)

sync_dep = declare_dependency(
  link_with : libsync
)

pkg.generate(libsync,
  filebase : 'libsync',
  name : 'hybris-sync',
  description : 'libhybris sync library',
  libraries : [libsync],
  version : lib_version
)
