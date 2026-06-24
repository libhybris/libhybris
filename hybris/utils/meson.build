executable('getprop',
  'getprop.c',
  include_directories: header_inc,
  dependencies : [hybris_dep, hybris_common_dep, android_properties_dep],
  link_with : libandroid_properties,
  pie : true,
  install : true
)

executable('setprop',
  'setprop.c',
  include_directories: header_inc,
  dependencies : [hybris_dep, hybris_common_dep, android_properties_dep],
  link_with : libandroid_properties,
  pie : true,
  install : true
)
