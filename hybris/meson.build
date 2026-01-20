project('libhybris', 'c', 'cpp',
  version : '0.1.0',
  default_options : [ 'warning_level=1',
                      'buildtype=debugoptimized' ])

api_version = '1.0'
lib_version = '0.1.0'
so_version = '0'

gstdroid_version = meson.project_version()
gstdroid_name = meson.project_name()

cc = meson.get_compiler('c')

libhybris_conf = configuration_data()

# Check wayland dependencies
want_wayland = get_option('wayland')
if want_wayland
  wayland_egl_req = '>= 1.15'
  wayland_client_dep = dependency('wayland-client', required : get_option('wayland'))
  wayland_egl_dep = dependency('wayland-egl', version : wayland_egl_req, required : get_option('wayland'))
  wayland_server_dep = dependency('wayland-server', required : get_option('wayland'))
  libhybris_conf.set('WANT_WAYLAND', 1)
  if not get_option('wayland_serverside_buffers')
    libhybris_conf.set('HYBRIS_NO_SERVER_SIDE_BUFFERS', 1)
    hybris_no_server_side_buffers = true
  else
    hybris_no_server_side_buffers = false
  endif
endif

libhybris_conf.set_quoted('PACKAGE', 'libhybris')
libhybris_conf.set_quoted('VERSION', '@0@'.format(lib_version))
libhybris_conf.set_quoted('PACKAGE_NAME', 'libhybris')
libhybris_conf.set_quoted('PACKAGE_STRING', 'libhybris @0@'.format(lib_version))

# configuration
if get_option('adreno_quirks')
  libhybris_conf.set('WANT_ADRENO_QUIRKS', 1)
endif
if get_option('arm_tracing')
  libhybris_conf.set('WANT_ARM_TRACING', 1)
endif
if get_option('enable_debug')
  libhybris_conf.set('WANT_DEBUG', 1)
endif
if get_option('glvnd')
  gl_lib_suffix = '_libhybris'
  libhybris_conf.set('WANT_GLVND', 1)
  glvnd_dep = dependency('libglvnd', required : true)
  ext_egl_dep = dependency('egl', required : true)
  ext_glesv2_dep = dependency('glesv2', required : true)
  want_glvnd = true
else
  gl_lib_suffix = ''
  want_glvnd = false
endif
if get_option('property_cache')
  libhybris_conf.set('WANT_RUNTIME_PROPERTY_CACHE', 1)
endif
if get_option('enable_trace')
  libhybris_conf.set('WANT_TRACE', 1)
endif
if get_option('ubuntu_camera_headers')
  libhybris_conf.set('WANT_UBUNTU_CAMERA_HEADERS', 1)
endif
if get_option('ubuntu_linker_overrides')
  libhybris_conf.set('UBUNTU_LINKER_OVERRIDES', 1)
endif

libhybris_conf.set('_GNU_SOURCE', '1')

if get_option('android_headers_dir') == ''
  hybris_dep = dependency('android-headers', required : true)
  android_headers_pkgconfig = 'android-headers'
else
  hybris_dep = declare_dependency(
    compile_args : '-I@0@'.format(get_option('android_headers_dir'))
  )
  android_headers_pkgconfig = ''
endif

check_headers = [
  ['hardware/hwcomposer2.h', 'HAS_HWCOMPOSER2_HEADERS'],
  ['hardware/gralloc1.h', 'HAS_GRALLOC1_HEADER'],
  ['hardware_legacy/vibrator.h', 'HAS_VIBRATOR_HEADER'],
  ['hardware_legacy/wifi.h', 'HAS_WIFI_LEGACY_HEADERS'],
  ['libnfc-nxp/phLibNfc.h', 'HAS_LIBNFC_NXP_HEADERS']
]

foreach h : check_headers
  if cc.has_header(h.get(0), dependencies : hybris_dep)
    libhybris_conf.set(h.get(1), 1)
  endif
endforeach

if cc.has_header('hardware/hwcomposer2.h', dependencies : hybris_dep)
  has_hwcomposer2_headers = true
else
  has_hwcomposer2_headers = false
endif

arch = get_option('arch')

android_headers_major = cc.get_define('ANDROID_VERSION_MAJOR', prefix : '#include <android-version.h>', dependencies : [hybris_dep]).to_int()
android_headers_minor = cc.get_define('ANDROID_VERSION_MINOR', prefix : '#include <android-version.h>', dependencies : [hybris_dep]).to_int()
android_headers_patch = cc.get_define('ANDROID_VERSION_PATCH', prefix : '#include <android-version.h>', dependencies : [hybris_dep]).to_int()

if get_option('default_hybris_ld_library_path') != ''
  default_hybris_ld_library_path = get_option('default_hybris_ld_library_path')
else
  if (android_headers_major >= 7)
    if arch == 'arm' or arch == 'x86'
      default_hybris_ld_library_path = '/system/lib:/odm/lib:/vendor/lib'
    else
      default_hybris_ld_library_path = '/system/lib64:/odm/lib64:/vendor/lib64'
    endif
  else
    if arch == 'arm' or arch == 'x86'
      default_hybris_ld_library_path = '/vendor/lib:/system/lib:/odm/lib'
    else
      default_hybris_ld_library_path = '/vendor/lib64:/system/lib64:/odm/lib64'
    endif
  endif
endif

root_dir = include_directories('.')
config_inc = include_directories('.')
header_inc = include_directories('include')

pkg = import('pkgconfig')

subdir('include')
subdir('common')
subdir('properties')
subdir('hardware')
subdir('ui')
subdir('gralloc')

if (android_headers_major >= 4 and android_headers_minor >= 2) or android_headers_major >= 5
  subdir('libsync')
endif

subdir('platforms')

subdir('camera')
subdir('egl')
subdir('glesv1')
subdir('glesv2')
subdir('hwc2')
subdir('input')
subdir('libnfc_ndef_nxp')
subdir('libnfc_nxp')
subdir('media')
subdir('opencl')
subdir('sf')
subdir('utils')
subdir('vibrator')
if cc.has_header('vulkan/vulkan.h')
  want_vulkan = true
  subdir('vulkan')
endif
subdir('wifi')

subdir('tests')

configure_file(output: 'config.h', configuration: libhybris_conf)
