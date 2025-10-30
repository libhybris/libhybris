libnfc_nxp = library('nfc_nxp',
  'libnfc_nxp.c',
  c_args : ['-Wno-error=unused-function'],
  dependencies : hybris_dep,
  include_directories: [header_inc],
  version : '1.0.0',
  install : true
)

nfc_nxp_dep = declare_dependency(
  link_with : libnfc_nxp
)

pkg.generate(libnfc_nxp,
  name : 'libnfc_nxp',
  description : 'libhybris NFC library (NXP variant)',
  libraries : [libnfc_nxp],
  version : lib_version
)
