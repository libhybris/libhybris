test_common_dep = declare_dependency(
  sources : 'test_common.cpp',
  compile_args : '-std=gnu++11',
  include_directories: [config_inc, header_inc],
  dependencies : [
    egl_dep,
    glesv2_dep,
    hybris_common_dep,
  ],
)

test_common_hwcomposer_deps = [
  hardware_dep,
  hybris_hwcomposerwindow_dep,
  sync_dep,
  test_common_dep,
]

if has_hwcomposer2_headers
  test_common_hwcomposer_deps += [hwc2_dep]
endif

test_common_hwcomposer_dep = declare_dependency(
  compile_args : ['-DUSE_HWCOMPOSER'],
  dependencies : test_common_hwcomposer_deps,
)

executable('test_audio',
  'test_audio.c',
  c_args : '-std=gnu99',
  include_directories: header_inc,
  dependencies : [hardware_dep, hybris_common_dep],
  pie : true,
  install : true
)

if want_wayland
  executable('test_camera',
    'test_camera.cpp',
    dependencies : [
      camera_dep,
      is_dep,
      test_common_dep,
      wayland_client_dep,
      wayland_egl_dep,
    ],
    pie : true,
    install : true
  )
endif

executable('test_dlopen',
  'test_dlopen.c',
  include_directories: [header_inc],
  dependencies : [hybris_common_dep],
  pie : true,
  install : true
)

executable('test_egl',
  'test_egl.cpp',
  cpp_args : '-std=gnu++11',
  include_directories: [header_inc],
  dependencies : [
    egl_dep,
    hybris_common_dep,
    hybris_eglplatformcommon_dep,
  ],
  pie : true,
  install : true
)

executable('test_egl_configs',
  'test_egl_configs.c',
  include_directories: [header_inc],
  dependencies : [
    egl_dep,
    hybris_common_dep,
  ],
  pie : true,
  install : true
)

executable('test_glesv2',
  'test_glesv2.cpp',
  dependencies : [
    test_common_hwcomposer_dep,
  ],
  pie : true,
  install : true
)

executable('test_glesv3',
  'test_glesv3.cpp',
  dependencies : [
    test_common_hwcomposer_dep,
  ],
  pie : true,
  install : true
)

executable('test_gps',
  'test_gps.c',
  include_directories: [header_inc],
  dependencies : [
    hardware_dep,
    hybris_common_dep,
    thread_dep,
  ],
  pie : true,
  install : true
)

executable('test_hwcomposer',
  'test_hwcomposer.cpp',
  dependencies : [
    test_common_hwcomposer_dep,
  ],
  pie : true,
  install : true
)

executable('test_input',
  'test_input.c',
  include_directories: [header_inc],
  dependencies : [
    hybris_common_dep,
    is_dep,
  ],
  pie : true,
  install : true
)

executable('test_lights',
  'test_lights.c',
  include_directories: [header_inc],
  dependencies : [
    hardware_dep,
    hybris_common_dep,
  ],
  pie : true,
  install : true
)

executable('test_media',
  'test_media.cpp',
  dependencies : [
    media_dep,
    sf_dep,
    test_common_dep,
  ],
  pie : true,
  install : true
)

executable('test_nfc',
  'test_nfc.c',
  include_directories: [header_inc],
  dependencies : [
    hardware_dep,
    hybris_common_dep,
    nfc_nxp_dep,
  ],
  pie : true,
  install : true
)

executable('test_opencl',
  'test_opencl.c',
  include_directories: [header_inc],
  dependencies : [
    hybris_common_dep,
    opencl_dep,
  ],
  pie : true,
  install : true
)

executable('test_recorder',
  'test_recorder.cpp',
  dependencies : [
    camera_dep,
    is_dep,
    media_dep,
    sf_dep,
    test_common_hwcomposer_dep,
  ],
  pie : true,
  install : true
)

executable('test_sensors',
  'test_sensors.c',
  include_directories: [header_inc],
  dependencies : [
    hardware_dep,
    hybris_common_dep,
  ],
  pie : true,
  install : true
)

executable('test_sf',
  'test_sf.cpp',
  dependencies : [
    sf_dep,
    test_common_dep,
  ],
  pie : true,
  install : true
)

executable('test_vibrator',
  'test_vibrator.c',
  include_directories: [header_inc],
  dependencies : [
    hardware_dep,
    hybris_common_dep,
  ],
  pie : true,
  install : true
)

if want_wayland and want_vulkan
  executable('test_vulkan',
    'test_vulkan.cpp',
    include_directories: [header_inc],
    dependencies : [
      hybris_common_dep,
      vulkan_dep,
      wayland_client_dep,
      wayland_egl_dep,
    ],
    pie : true,
    install : true
  )
endif

executable('test_wifi',
  'test_wifi.c',
  include_directories: [header_inc],
  dependencies : [
    hybris_common_dep,
    wifi_dep,
  ],
  pie : true,
  install : true
)
