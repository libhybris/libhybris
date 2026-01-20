q_sources = [
  'hybris_compat.cpp',
  'bionic/libc/bionic/bionic_allocator.cpp',
  'bionic/libc/bionic/bionic_call_ifunc_resolver.cpp',
  'bionic/libc/bionic/bionic_elf_tls.cpp',
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
  'linker_tls.cpp',
  'linker_utils.cpp',
  'rt.cpp',
  '../strlcpy.c',
  '../strlcat.c',
]

if get_option('arch') == 'arm64'
  q_sources += [
    'tlsdesc_resolver.S',
  ]
endif

q_cpp_args = [
  '-fno-stack-protector',
  '-Wstrict-overflow=5',
  '-std=gnu++11',
  '-Wold-style-cast',
  '-D_USING_LIBCXX',
  '-DDEFAULT_HYBRIS_LD_LIBRARY_PATH="@0@"'.format(default_hybris_ld_library_path)
]

q = both_libraries('q',
  q_sources,
  include_directories : [header_inc, config_inc, common_inc, 'bionic/libc', 'bionic/libc/include'],
  c_args : '-std=gnu99',
  cpp_args : q_cpp_args,
  dependencies : [hybris_dep, thread_dep],
  name_prefix : '',
  install_dir : get_option('libdir') / 'libhybris/linker',
  install : true
)
