libui = library('ui',
  'ui.c',
  include_directories: header_inc,
  dependencies : [hybris_dep, hybris_common_dep],
  version : '1.0.0',
  install : true
)

ui_dep = declare_dependency(
  link_with : libui
)
