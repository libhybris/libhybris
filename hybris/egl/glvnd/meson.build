prog_python = import('python').find_installation('python3')

glvnd_xml_files = ['@CURRENT_SOURCE_DIR@/generate/egl.xml', '@CURRENT_SOURCE_DIR@/generate/egl_other.xml', '@CURRENT_SOURCE_DIR@/generate/egl_hybris.xml']

g_egldispatchstubs_c = custom_target(
  'g_egldispatchstubs.c',
  capture : true,
  output : 'g_egldispatchstubs.c',
  input : 'generate/gen_egl_dispatch.py',
  command : [prog_python, '@INPUT@', 'source', glvnd_xml_files],
)

g_egldispatchstubs_h = custom_target(
  'g_egldispatchstubs.h',
  capture : true,
  output : 'g_egldispatchstubs.h',
  input : 'generate/gen_egl_dispatch.py',
  command : [prog_python, '@INPUT@', 'header', glvnd_xml_files],
)
