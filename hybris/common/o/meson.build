o_sources = [
  'hybris_compat.cpp',
  'dlfcn.cpp',
  'linker_allocator.cpp',
  'linker_block_allocator.cpp',
  'linker.cpp',
  'linker_cfi.cpp',
  'linker_config.cpp',
  'linker_dlwarning.cpp',
  'linker_gdb_support.cpp',
  'linker_globals.cpp',
  'linker_logger.cpp',
  'linker_main.cpp',
  'linker_mapped_file_fragment.cpp',
  'linker_memory.cpp',
  'linker_namespaces.cpp',
  'linker_phdr.cpp',
  'linker_sdk_versions.cpp',
  'linker_soinfo.cpp',
  'linker_utils.cpp',
  'rt.cpp',
  '../strlcpy.c',
  '../strlcat.c',
]

o_cpp_args = [
  '-fno-stack-protector',
  '-Wstrict-overflow=5',
  '-std=gnu++11',
  '-Wold-style-cast',
  '-DDEFAULT_HYBRIS_LD_LIBRARY_PATH="@0@"'.format(default_hybris_ld_library_path)
]

o = both_libraries('o',
  o_sources,
  include_directories : [header_inc, config_inc, common_inc, 'bionic/libc', 'bionic/libc/include'],
  c_args : '-std=gnu99',
  cpp_args : o_cpp_args,
  dependencies : hybris_dep,
  name_prefix : '',
  install_dir : get_option('libdir') / 'libhybris/linker',
  install : true
)
