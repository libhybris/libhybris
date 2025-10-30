libnfc_ndef_nxp = library('nfc_ndef_nxp',
  'libnfc_ndef_nxp.c',
  c_args : ['-Wno-error=unused-function'],
  dependencies : hybris_dep,
  include_directories: [header_inc],
  version : '1.0.0',
  install : true
)

nfc_ndef_nxp_dep = declare_dependency(
  link_with : libnfc_ndef_nxp
)

pkg.generate(libnfc_ndef_nxp,
  name : 'libnfc_ndef_nxp',
  description : 'libhybris NFC NDEF library (NXP variant)',
  libraries : [libnfc_ndef_nxp],
  version : lib_version
)
