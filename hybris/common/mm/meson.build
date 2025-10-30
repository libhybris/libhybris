mm_sources = [
  'hybris_compat.cpp',
  'dlfcn.cpp',
  'linker_allocator.cpp',
  'linker_block_allocator.cpp',
  'linker.cpp',
  'linker_memory.cpp',
  'linker_phdr.cpp',
  'linker_sdk_versions.cpp',
  'rt.cpp',
  '../strlcpy.c',
  '../strlcat.c',
]

mm_cpp_args = [
  '-fno-stack-protector',
  '-Wstrict-overflow=5',
  '-std=gnu++11',
  '-Wold-style-cast',
  '-DDEFAULT_HYBRIS_LD_LIBRARY_PATH="@0@"'.format(default_hybris_ld_library_path)
]

mm = both_libraries('mm',
  mm_sources,
  include_directories : [header_inc, config_inc, common_inc, 'bionic/libc', 'bionic/libc/include'],
  c_args : '-std=gnu99',
  cpp_args : mm_cpp_args,
  dependencies : hybris_dep,
  name_prefix : '',
  install_dir : get_option('libdir') / 'libhybris/linker',
  install : true
)
