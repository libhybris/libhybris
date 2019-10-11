/*
 * Copyright (C) 2016 The Android Open Source Project
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "hybris_compat.h"

#include "linker_main.h"

#include <link.h>
#include <sys/auxv.h>
#include <sys/cdefs-android.h>
#include <android-version.h>
#include <stdarg.h>

#include "linker_debug.h"
#include "linker_cfi.h"
#include "linker_gdb_support.h"
#include "linker_globals.h"
#include "linker_phdr.h"
#include "linker_tls.h"
#include "linker_utils.h"

#include "private/bionic_globals.h"
#include "private/bionic_tls.h"
#include "private/KernelArgumentBlock.h"

#include "android-base/unique_fd.h"
//#include "android-base/strings.h"
//#include "android-base/stringprintf.h"

#include "linker_utils.h"

#ifdef __ANDROID__
#include "debuggerd/handler.h"
#endif

#include <async_safe/log.h>
#include <bionic/libc_init_common.h>
#include <bionic/pthread_internal.h>

#include <vector>

extern void __libc_init_globals(KernelArgumentBlock&);
#ifdef DISABLED_FOR_HYBRIS_SUPPORT
extern void __libc_init_AT_SECURE(KernelArgumentBlock&);
#endif


#ifdef DISABLED_FOR_HYBRIS_SUPPORT
extern "C" void _start();
#endif

static ElfW(Addr) get_elf_exec_load_bias(const ElfW(Ehdr)* elf);

static void get_elf_base_from_phdr(const ElfW(Phdr)* phdr_table, size_t phdr_count,
                                   ElfW(Addr)* base, ElfW(Addr)* load_bias);

// These should be preserved static to avoid emitting
// RELATIVE relocations for the part of the code running
// before linker links itself.

// TODO (dimtiry): remove somain, rename solist to solist_head
static soinfo* solist;
static soinfo* sonext;
static soinfo* somain; // main process, always the one after libdl_info
static soinfo* solinker;
static soinfo* vdso; // vdso if present

void solist_add_soinfo(soinfo* si) {
  sonext->next = si;
  sonext = si;
}

bool solist_remove_soinfo(soinfo* si) {
  soinfo *prev = nullptr, *trav;
  for (trav = solist; trav != nullptr; trav = trav->next) {
    if (trav == si) {
      break;
    }
    prev = trav;
  }

  if (trav == nullptr) {
    // si was not in solist
    PRINT("name \"%s\"@%p is not in solist!", si->get_realpath(), si);
    return false;
  }

  // prev will never be null, because the first entry in solist is
  // always the static libdl_info.
  CHECK(prev != nullptr);
  prev->next = si->next;
  if (si == sonext) {
    sonext = prev;
  }

  return true;
}

soinfo* solist_get_head() {
  return solist;
}

soinfo* solist_get_somain() {
  return somain;
}

soinfo* solist_get_vdso() {
  return vdso;
}

int g_ld_debug_verbosity;

static std::vector<std::string> g_ld_preload_names;

static std::vector<soinfo*> g_ld_preloads;

static void parse_path(const char* path, const char* delimiters,
                       std::vector<std::string>* resolved_paths) {
  std::vector<std::string> paths;
  split_path(path, delimiters, &paths);
  resolve_paths(paths, resolved_paths);
}

static void parse_LD_LIBRARY_PATH(const char* path) {
  std::vector<std::string> ld_libary_paths;
  parse_path(path, ":", &ld_libary_paths);
  g_default_namespace->set_ld_library_paths(std::move(ld_libary_paths));
}

static void parse_LD_PRELOAD(const char* path) {
  g_ld_preload_names.clear();
  if (path != nullptr) {
    // We have historically supported ':' as well as ' ' in LD_PRELOAD.
    g_ld_preload_names = split(path, " :");
    g_ld_preload_names.erase(std::remove_if(g_ld_preload_names.begin(), g_ld_preload_names.end(),
                                            [](const std::string& s) { return s.empty(); }),
                             g_ld_preload_names.end());
  }
}

// An empty list of soinfos
static soinfo_list_t g_empty_list;

static void add_vdso() {
  ElfW(Ehdr)* ehdr_vdso = reinterpret_cast<ElfW(Ehdr)*>(getauxval(AT_SYSINFO_EHDR));
  if (ehdr_vdso == nullptr) {
    return;
  }

  soinfo* si = soinfo_alloc(g_default_namespace, "[vdso]", nullptr, 0, 0);

  si->phdr = reinterpret_cast<ElfW(Phdr)*>(reinterpret_cast<char*>(ehdr_vdso) + ehdr_vdso->e_phoff);
  si->phnum = ehdr_vdso->e_phnum;
  si->base = reinterpret_cast<ElfW(Addr)>(ehdr_vdso);
  si->size = phdr_table_get_load_size(si->phdr, si->phnum);
  si->load_bias = get_elf_exec_load_bias(ehdr_vdso);

  si->prelink_image();
  si->link_image(g_empty_list, soinfo_list_t::make_list(si), nullptr, nullptr);
  // prevents accidental unloads...
  si->set_dt_flags_1(si->get_dt_flags_1() | DF_1_NODELETE);
  si->set_linked();
  si->call_constructors();

  vdso = si;
}

// Initializes an soinfo's link_map_head field using other fields from the
// soinfo (phdr, phnum, load_bias).
static void init_link_map_head(soinfo& info, const char* linker_path) {
  auto& map = info.link_map_head;
  map.l_addr = info.load_bias;
  map.l_name = const_cast<char*>(linker_path);
  phdr_table_get_dynamic_section(info.phdr, info.phnum, info.load_bias, &map.l_ld, nullptr);
}

extern "C" int __system_properties_init(void);

struct ExecutableInfo {
  std::string path;
  struct stat file_stat;
  const ElfW(Phdr)* phdr;
  size_t phdr_count;
  ElfW(Addr) entry_point;
};

static ExecutableInfo get_executable_info() {
  ExecutableInfo result = {};

  if (is_first_stage_init()) {
    // /proc fs is not mounted when first stage init starts. Therefore we can't
    // use /proc/self/exe for init.
    stat("/init", &result.file_stat);

    // /init may be a symlink, so try to read it as such.
    char path[PATH_MAX];
    ssize_t path_len = readlink("/init", path, sizeof(path));
    if (path_len == -1 || path_len >= static_cast<ssize_t>(sizeof(path))) {
      result.path = "/init";
    } else {
      result.path = std::string(path, path_len);
    }
  } else {
    // Stat "/proc/self/exe" instead of executable_path because
    // the executable could be unlinked by this point and it should
    // not cause a crash (see http://b/31084669)
    if (TEMP_FAILURE_RETRY(stat("/proc/self/exe", &result.file_stat)) != 0) {
      async_safe_fatal("unable to stat \"/proc/self/exe\": %s", strerror(errno));
    }
    char path[PATH_MAX];
    ssize_t path_len = readlink("/proc/self/exe", path, sizeof(path));
    if (path_len == -1 || path_len >= static_cast<ssize_t>(sizeof(path))) {
      async_safe_fatal("readlink('/proc/self/exe') failed: %s", strerror(errno));
    }
    result.path = std::string(path, path_len);
  }

  result.phdr = reinterpret_cast<const ElfW(Phdr)*>(getauxval(AT_PHDR));
  result.phdr_count = getauxval(AT_PHNUM);
  result.entry_point = getauxval(AT_ENTRY);
  return result;
}

#if defined(__LP64__)
static char kLinkerPath[] = "/system/bin/linker64";
#else
static char kLinkerPath[] = "/system/bin/linker";
#endif

__printflike(1, 2)
static void __linker_error(const char* fmt, ...) {
  va_list ap;

  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  va_end(ap);

  va_start(ap, fmt);
  async_safe_format_log_va_list(ANDROID_LOG_FATAL, "linker", fmt, ap);
  va_end(ap);

  _exit(EXIT_FAILURE);
}

static void __linker_cannot_link(const char* argv0) {
  __linker_error("CANNOT LINK EXECUTABLE \"%s\": %s\n",
                 argv0,
                 linker_get_error_buffer());
}

// Load an executable. Normally the kernel has already loaded the executable when the linker
// starts. The linker can be invoked directly on an executable, though, and then the linker must
// load it. This function doesn't load dependencies or resolve relocations.
static ExecutableInfo load_executable(const char* orig_path) {
  ExecutableInfo result = {};

  if (orig_path[0] != '/') {
    __linker_error("error: expected absolute path: \"%s\"\n", orig_path);
  }

  off64_t file_offset;
  android::base::unique_fd fd(open_executable(orig_path, &file_offset, &result.path));
  if (fd.get() == -1) {
    __linker_error("error: unable to open file \"%s\"\n", orig_path);
  }

  if (TEMP_FAILURE_RETRY(fstat(fd.get(), &result.file_stat)) == -1) {
    __linker_error("error: unable to stat \"%s\": %s\n", result.path.c_str(), strerror(errno));
  }

  ElfReader elf_reader;
  if (!elf_reader.Read(result.path.c_str(), fd.get(), file_offset, result.file_stat.st_size)) {
    __linker_error("error: %s\n", linker_get_error_buffer());
  }
  address_space_params address_space;
  if (!elf_reader.Load(&address_space)) {
    __linker_error("error: %s\n", linker_get_error_buffer());
  }

  result.phdr = elf_reader.loaded_phdr();
  result.phdr_count = elf_reader.phdr_count();
  result.entry_point = elf_reader.entry_point();
  return result;
}

static ElfW(Addr) linker_main(KernelArgumentBlock& args, const char* exe_to_load) {
  ProtectedDataGuard guard;

#if TIMING
  struct timeval t0, t1;
  gettimeofday(&t0, 0);
#endif

#ifdef DISABLED_FOR_HYBRIS_SUPPORT
  // Sanitize the environment.
  __libc_init_AT_SECURE(args.envp);

  // Initialize system properties
  __system_properties_init(); // may use 'environ'

  // Register the debuggerd signal handler.
#ifdef __ANDROID__
  debuggerd_callbacks_t callbacks = {
    .get_abort_message = []() {
      return __libc_shared_globals()->abort_msg;
    },
    .post_dump = &notify_gdb_of_libraries,
  };
  debuggerd_init(&callbacks);
#endif
#endif

  g_linker_logger.ResetState();

  // Get a few environment variables.
  const char* LD_DEBUG = getenv("LD_DEBUG");
  if (LD_DEBUG != nullptr) {
    g_ld_debug_verbosity = atoi(LD_DEBUG);
  }

#if defined(__LP64__)
  INFO("[ Android dynamic linker (64-bit) ]");
#else
  INFO("[ Android dynamic linker (32-bit) ]");
#endif

  // These should have been sanitized by __libc_init_AT_SECURE, but the test
  // doesn't cost us anything.
  const char* ldpath_env = nullptr;
  const char* ldpreload_env = nullptr;
  if (!getauxval(AT_SECURE)) {
    ldpath_env = getenv("LD_LIBRARY_PATH");
    if (ldpath_env != nullptr) {
      INFO("[ LD_LIBRARY_PATH set to \"%s\" ]", ldpath_env);
    }
    ldpreload_env = getenv("LD_PRELOAD");
    if (ldpreload_env != nullptr) {
      INFO("[ LD_PRELOAD set to \"%s\" ]", ldpreload_env);
    }
  }

  const ExecutableInfo exe_info = exe_to_load ? load_executable(exe_to_load) :
                                                get_executable_info();

  // Assign to a static variable for the sake of the debug map, which needs
  // a C-style string to last until the program exits.
  static std::string exe_path = exe_info.path;

  INFO("[ Linking executable \"%s\" ]", exe_path.c_str());

  // Initialize the main exe's soinfo.
  soinfo* si = soinfo_alloc(g_default_namespace,
                            exe_path.c_str(), &exe_info.file_stat,
                            0, RTLD_GLOBAL);
  somain = si;
  si->phdr = exe_info.phdr;
  si->phnum = exe_info.phdr_count;
  get_elf_base_from_phdr(si->phdr, si->phnum, &si->base, &si->load_bias);
  si->size = phdr_table_get_load_size(si->phdr, si->phnum);
  si->dynamic = nullptr;
  si->set_main_executable();
  init_link_map_head(*si, exe_path.c_str());

  // Register the main executable and the linker upfront to have
  // gdb aware of them before loading the rest of the dependency
  // tree.
  //
  // gdb expects the linker to be in the debug shared object list.
  // Without this, gdb has trouble locating the linker's ".text"
  // and ".plt" sections. Gdb could also potentially use this to
  // relocate the offset of our exported 'rtld_db_dlactivity' symbol.
  //
  insert_link_map_into_debug_map(&si->link_map_head);
  insert_link_map_into_debug_map(&solinker->link_map_head);

  add_vdso();

  ElfW(Ehdr)* elf_hdr = reinterpret_cast<ElfW(Ehdr)*>(si->base);

  // We haven't supported non-PIE since Lollipop for security reasons.
  if (elf_hdr->e_type != ET_DYN) {
    // We don't use async_safe_fatal here because we don't want a tombstone:
    // even after several years we still find ourselves on app compatibility
    // investigations because some app's trying to launch an executable that
    // hasn't worked in at least three years, and we've "helpfully" dropped a
    // tombstone for them. The tombstone never provided any detail relevant to
    // fixing the problem anyway, and the utility of drawing extra attention
    // to the problem is non-existent at this late date.
    async_safe_format_fd(STDERR_FILENO,
                         "\"%s\": error: Android 5.0 and later only support "
                         "position-independent executables (-fPIE).\n",
                         g_argv[0]);
    exit(EXIT_FAILURE);
  }

  // Use LD_LIBRARY_PATH and LD_PRELOAD (but only if we aren't setuid/setgid).
  parse_LD_LIBRARY_PATH(ldpath_env);
  parse_LD_PRELOAD(ldpreload_env);

  std::vector<android_namespace_t*> namespaces = init_default_namespaces(exe_path.c_str());

  if (!si->prelink_image()) __linker_cannot_link(g_argv[0]);

  // add somain to global group
  si->set_dt_flags_1(si->get_dt_flags_1() | DF_1_GLOBAL);
  // ... and add it to all other linked namespaces
  for (auto linked_ns : namespaces) {
    if (linked_ns != g_default_namespace) {
      linked_ns->add_soinfo(somain);
      somain->add_secondary_namespace(linked_ns);
    }
  }

  linker_setup_exe_static_tls(g_argv[0]);

  // Load ld_preloads and dependencies.
  std::vector<const char*> needed_library_name_list;
  size_t ld_preloads_count = 0;

  for (const auto& ld_preload_name : g_ld_preload_names) {
    needed_library_name_list.push_back(ld_preload_name.c_str());
    ++ld_preloads_count;
  }

  for_each_dt_needed(si, [&](const char* name) {
    needed_library_name_list.push_back(name);
  });

  const char** needed_library_names = &needed_library_name_list[0];
  size_t needed_libraries_count = needed_library_name_list.size();

  if (needed_libraries_count > 0 &&
      !find_libraries(g_default_namespace,
                      si,
                      needed_library_names,
                      needed_libraries_count,
                      nullptr,
                      &g_ld_preloads,
                      ld_preloads_count,
                      RTLD_GLOBAL,
                      nullptr,
                      true /* add_as_children */,
                      true /* search_linked_namespaces */,
                      &namespaces)) {
    __linker_cannot_link(g_argv[0]);
  } else if (needed_libraries_count == 0) {
    if (!si->link_image(g_empty_list, soinfo_list_t::make_list(si), nullptr, nullptr)) {
      __linker_cannot_link(g_argv[0]);
    }
    si->increment_ref_count();
  }

  linker_finalize_static_tls();

#ifdef DISABLED_FOR_HYBRIS_SUPPORT
  __libc_init_main_thread_final();
#endif

  if (!get_cfi_shadow()->InitialLinkDone(solist)) __linker_cannot_link(g_argv[0]);

  si->call_pre_init_constructors();
  si->call_constructors();

#if TIMING
  gettimeofday(&t1, nullptr);
  PRINT("LINKER TIME: %s: %d microseconds", g_argv[0], (int) (
           (((long long)t1.tv_sec * 1000000LL) + (long long)t1.tv_usec) -
           (((long long)t0.tv_sec * 1000000LL) + (long long)t0.tv_usec)));
#endif
#if STATS
  PRINT("RELO STATS: %s: %d abs, %d rel, %d copy, %d symbol", g_argv[0],
         linker_stats.count[kRelocAbsolute],
         linker_stats.count[kRelocRelative],
         linker_stats.count[kRelocCopy],
         linker_stats.count[kRelocSymbol]);
#endif
#if COUNT_PAGES
  {
    unsigned n;
    unsigned i;
    unsigned count = 0;
    for (n = 0; n < 4096; n++) {
      if (bitmask[n]) {
        unsigned x = bitmask[n];
#if defined(__LP64__)
        for (i = 0; i < 32; i++) {
#else
        for (i = 0; i < 8; i++) {
#endif
          if (x & 1) {
            count++;
          }
          x >>= 1;
        }
      }
    }
    PRINT("PAGES MODIFIED: %s: %d (%dKB)", g_argv[0], count, count * 4);
  }
#endif

#if TIMING || STATS || COUNT_PAGES
  fflush(stdout);
#endif

  // We are about to hand control over to the executable loaded.  We don't want
  // to leave dirty pages behind unnecessarily.
  purge_unused_memory();

  ElfW(Addr) entry = exe_info.entry_point;
  TRACE("[ Ready to execute \"%s\" @ %p ]", si->get_realpath(), reinterpret_cast<void*>(entry));
  return entry;
}

/* Compute the load-bias of an existing executable. This shall only
 * be used to compute the load bias of an executable or shared library
 * that was loaded by the kernel itself.
 *
 * Input:
 *    elf    -> address of ELF header, assumed to be at the start of the file.
 * Return:
 *    load bias, i.e. add the value of any p_vaddr in the file to get
 *    the corresponding address in memory.
 */
static ElfW(Addr) get_elf_exec_load_bias(const ElfW(Ehdr)* elf) {
  ElfW(Addr) offset = elf->e_phoff;
  const ElfW(Phdr)* phdr_table =
      reinterpret_cast<const ElfW(Phdr)*>(reinterpret_cast<uintptr_t>(elf) + offset);
  const ElfW(Phdr)* phdr_end = phdr_table + elf->e_phnum;

  for (const ElfW(Phdr)* phdr = phdr_table; phdr < phdr_end; phdr++) {
    if (phdr->p_type == PT_LOAD) {
      return reinterpret_cast<ElfW(Addr)>(elf) + phdr->p_offset - phdr->p_vaddr;
    }
  }
  return 0;
}

/* Find the load bias and base address of an executable or shared object loaded
 * by the kernel. The ELF file's PHDR table must have a PT_PHDR entry.
 *
 * A VDSO doesn't have a PT_PHDR entry in its PHDR table.
 */
static void get_elf_base_from_phdr(const ElfW(Phdr)* phdr_table, size_t phdr_count,
                                   ElfW(Addr)* base, ElfW(Addr)* load_bias) {
  for (size_t i = 0; i < phdr_count; ++i) {
    if (phdr_table[i].p_type == PT_PHDR) {
      *load_bias = reinterpret_cast<ElfW(Addr)>(phdr_table) - phdr_table[i].p_vaddr;
      *base = reinterpret_cast<ElfW(Addr)>(phdr_table) - phdr_table[i].p_offset;
      return;
    }
  }
  async_safe_fatal("Could not find a PHDR: broken executable?");
}

#if defined(__i386__)
__LIBC_HIDDEN__ void* __libc_sysinfo;
#endif

#ifdef DISABLED_FOR_HYBRIS_SUPPORT
__LIBC_HIDDEN__ __attribute__((__naked__)) void __libc_int0x80() {
 __asm__ volatile("int $0x80; ret");
}
// Detect an attempt to run the linker on itself. e.g.:
//   /system/bin/linker64 /system/bin/linker64
// Use priority-1 to run this constructor before other constructors.
__attribute__((constructor(1))) static void detect_self_exec() {
  // Normally, the linker initializes the auxv global before calling its
  // constructors. If the linker loads itself, though, the first loader calls
  // the second loader's constructors before calling __linker_init.
  if (__libc_shared_globals()->auxv != nullptr) {
    return;
  }
#if defined(__i386__)
  // We don't have access to the auxv struct from here, so use the int 0x80
  // fallback.
  __libc_sysinfo = reinterpret_cast<void*>(__libc_int0x80);
#endif
  __linker_error("error: linker cannot load itself\n");
}
#endif

static ElfW(Addr) __attribute__((noinline))
__linker_init_post_relocation(KernelArgumentBlock& args, soinfo& linker_so);
#ifdef DISABLED_FOR_HYBRIS_SUPPORT
/*
 * This is the entry point for the linker, called from begin.S. This
 * method is responsible for fixing the linker's own relocations, and
 * then calling __linker_init_post_relocation().
 *
 * Because this method is called before the linker has fixed it's own
 * relocations, any attempt to reference an extern variable, extern
 * function, or other GOT reference will generate a segfault.
 */
extern "C" ElfW(Addr) __linker_init(void* raw_args) {
  // Initialize TLS early so system calls and errno work.
  KernelArgumentBlock args(raw_args);
  bionic_tcb temp_tcb = {};
  __libc_init_main_thread_early(args, &temp_tcb);

  // When the linker is run by itself (rather than as an interpreter for
  // another program), AT_BASE is 0.
  ElfW(Addr) linker_addr = getauxval(AT_BASE);
  if (linker_addr == 0) {
    // The AT_PHDR and AT_PHNUM aux values describe this linker instance, so use
    // the phdr to find the linker's base address.
    ElfW(Addr) load_bias;
    get_elf_base_from_phdr(
      reinterpret_cast<ElfW(Phdr)*>(getauxval(AT_PHDR)), getauxval(AT_PHNUM),
      &linker_addr, &load_bias);
  }

  ElfW(Ehdr)* elf_hdr = reinterpret_cast<ElfW(Ehdr)*>(linker_addr);
  ElfW(Phdr)* phdr = reinterpret_cast<ElfW(Phdr)*>(linker_addr + elf_hdr->e_phoff);

  soinfo tmp_linker_so(nullptr, nullptr, nullptr, 0, 0);

  tmp_linker_so.base = linker_addr;
  tmp_linker_so.size = phdr_table_get_load_size(phdr, elf_hdr->e_phnum);
  tmp_linker_so.load_bias = get_elf_exec_load_bias(elf_hdr);
  tmp_linker_so.dynamic = nullptr;
  tmp_linker_so.phdr = phdr;
  tmp_linker_so.phnum = elf_hdr->e_phnum;
  tmp_linker_so.set_linker_flag();

  // Prelink the linker so we can access linker globals.
  if (!tmp_linker_so.prelink_image()) __linker_cannot_link(args.argv[0]);

  // This might not be obvious... The reasons why we pass g_empty_list
  // in place of local_group here are (1) we do not really need it, because
  // linker is built with DT_SYMBOLIC and therefore relocates its symbols against
  // itself without having to look into local_group and (2) allocators
  // are not yet initialized, and therefore we cannot use linked_list.push_*
  // functions at this point.
  if (!tmp_linker_so.link_image(g_empty_list, g_empty_list, nullptr, nullptr)) __linker_cannot_link(args.argv[0]);

  return __linker_init_post_relocation(args, tmp_linker_so);
}
#endif

#ifdef DISABLED_FOR_HYBRIS_SUPPORT
/*
 * This code is called after the linker has linked itself and fixed its own
 * GOT. It is safe to make references to externs and other non-local data at
 * this point. The compiler sometimes moves GOT references earlier in a
 * function, so avoid inlining this function (http://b/80503879).
 */
static ElfW(Addr) __attribute__((noinline))
__linker_init_post_relocation(KernelArgumentBlock& args, soinfo& tmp_linker_so) {
  // Finish initializing the main thread.
  __libc_init_main_thread_late();

  // We didn't protect the linker's RELRO pages in link_image because we
  // couldn't make system calls on x86 at that point, but we can now...
  if (!tmp_linker_so.protect_relro()) __linker_cannot_link(args.argv[0]);

  // Initialize the linker's static libc's globals
  __libc_init_globals(args);

  // Initialize the linker's own global variables
  tmp_linker_so.call_constructors();

  // When the linker is run directly rather than acting as PT_INTERP, parse
  // arguments and determine the executable to load. When it's instead acting
  // as PT_INTERP, AT_ENTRY will refer to the loaded executable rather than the
  // linker's _start.
  const char* exe_to_load = nullptr;
  if (getauxval(AT_ENTRY) == reinterpret_cast<uintptr_t>(&_start)) {
    if (args.argc <= 1 || !strcmp(args.argv[1], "--help")) {
      async_safe_format_fd(STDOUT_FILENO,
         "Usage: %s program [arguments...]\n"
         "       %s path.zip!/program [arguments...]\n"
         "\n"
         "A helper program for linking dynamic executables. Typically, the kernel loads\n"
         "this program because it's the PT_INTERP of a dynamic executable.\n"
         "\n"
         "This program can also be run directly to load and run a dynamic executable. The\n"
         "executable can be inside a zip file if it's stored uncompressed and at a\n"
         "page-aligned offset.\n",
         args.argv[0], args.argv[0]);
      exit(0);
    }
    exe_to_load = args.argv[1];
    __libc_shared_globals()->initial_linker_arg_count = 1;
  }

  // store argc/argv/envp to use them for calling constructors
  g_argc = args.argc - __libc_shared_globals()->initial_linker_arg_count;
  g_argv = args.argv + __libc_shared_globals()->initial_linker_arg_count;
  g_envp = args.envp;
  __libc_shared_globals()->init_progname = g_argv[0];

  // Initialize static variables. Note that in order to
  // get correct libdl_info we need to call constructors
  // before get_libdl_info().
  sonext = solist = solinker = get_libdl_info(kLinkerPath, tmp_linker_so);
  g_default_namespace->add_soinfo(solinker);
  init_link_map_head(*solinker, kLinkerPath);

  ElfW(Addr) start_address = linker_main(args, exe_to_load);

  INFO("[ Jumping to _start (%p)... ]", reinterpret_cast<void*>(start_address));

  // Return the address that the calling assembly stub should jump to.
  return start_address;
}
#endif


static void generate_tmpsoinfo(soinfo& tmp_linker_so) {
  Dl_info dlinfo;
  int ret = dladdr((void *)generate_tmpsoinfo, &dlinfo);
  if(!ret) {
    PRINT("can't get self info:ret\n", ret);
    exit(-1);
  }
  ElfW(Addr) linker_addr = (ElfW(Addr))dlinfo.dli_fbase;
  DEBUG("get self base adder=%p\n", linker_addr);

  ElfW(Ehdr)* elf_hdr = reinterpret_cast<ElfW(Ehdr)*>(linker_addr);
  ElfW(Phdr)* phdr = reinterpret_cast<ElfW(Phdr)*>(linker_addr + elf_hdr->e_phoff);

  tmp_linker_so.base = linker_addr;
  tmp_linker_so.size = phdr_table_get_load_size(phdr, elf_hdr->e_phnum);
  tmp_linker_so.load_bias = get_elf_exec_load_bias(elf_hdr);
  tmp_linker_so.dynamic = nullptr;
  tmp_linker_so.phdr = phdr;
  tmp_linker_so.phnum = elf_hdr->e_phnum;
  tmp_linker_so.set_linker_flag();

  DEBUG("tmp_linker_so's load_bias=%p \n", tmp_linker_so.load_bias);

  // Prelink the linker so we can access linker globals.
  if (!tmp_linker_so.prelink_image()) {
    PRINT("can't prelink self:ret\n");
  };

  //tmp_linker_so.call_constructors();
}

#if (ANDROID_VERSION_MAJOR >= 10)
bionic_tls* __allocate_temp_bionic_tls() {
  size_t allocation_size = __BIONIC_ALIGN(sizeof(bionic_tls), PAGE_SIZE);
  void* allocation = mmap(nullptr, allocation_size,
                          PROT_READ | PROT_WRITE,
                          MAP_PRIVATE | MAP_ANONYMOUS,
                          -1, 0);
  if (allocation == MAP_FAILED) {
    // Avoid strerror because it might need bionic_tls.
    PRINT("failed to allocate bionic_tls: error %d", errno);
 }
  return static_cast<bionic_tls*>(allocation);
}
#endif

static const char* get_executable_path() {
  static std::string executable_path;
  if (executable_path.empty()) {
    char path[PATH_MAX];
    ssize_t path_len = readlink("/proc/self/exe", path, sizeof(path));
    if (path_len == -1 || path_len >= static_cast<ssize_t>(sizeof(path))) {
      async_safe_fatal("readlink('/proc/self/exe') failed: %s", strerror(errno));
    }
    executable_path = std::string(path, path_len);
  }

  return executable_path.c_str();
}

void* (*_get_hooked_symbol)(const char *sym, const char *requester);
void *(*_create_wrapper)(const char *symbol, void *function, int wrapper_type);
#ifdef WANT_ARM_TRACING
extern "C" void android_linker_init(int sdk_version, void* (*get_hooked_symbol)(const char*, const char*), int enable_linker_gdb_support, void *(create_wrapper)(const char*, void*, int)) {
#else
extern "C" void android_linker_init(int sdk_version, void* (*get_hooked_symbol)(const char*, const char*), int enable_linker_gdb_support) {
#endif
  // Get a few environment variables.
  const char* LD_DEBUG = getenv("HYBRIS_LD_DEBUG");
  if (LD_DEBUG != nullptr) {
    g_ld_debug_verbosity = atoi(LD_DEBUG);
  }

  const char* ldpath_env = nullptr;
  const char* ldpreload_env = nullptr;
  if (!getauxval(AT_SECURE)) {
    ldpath_env = getenv("HYBRIS_LD_LIBRARY_PATH");
    ldpreload_env = getenv("HYBRIS_LD_PRELOAD");
  }

  if (DEFAULT_HYBRIS_LD_LIBRARY_PATH)
    parse_LD_LIBRARY_PATH(DEFAULT_HYBRIS_LD_LIBRARY_PATH);
  else
    parse_LD_LIBRARY_PATH(ldpath_env);
  parse_LD_PRELOAD(ldpreload_env);

  DEBUG("sdk_version %d\n", sdk_version);

  if (sdk_version > 0)
    set_application_target_sdk_version(sdk_version);

  _get_hooked_symbol = get_hooked_symbol;
  _linker_enable_gdb_support = enable_linker_gdb_support;

  soinfo tmp_linker_so(nullptr, nullptr, nullptr, 0, 0);
  generate_tmpsoinfo(tmp_linker_so);

  sonext = solist = get_libdl_info(kLinkerPath, tmp_linker_so);

  init_link_map_head(tmp_linker_so, kLinkerPath);
  insert_link_map_into_debug_map(&tmp_linker_so.link_map_head);

  DEBUG("sdk_version %d\n", sdk_version);

  init_default_namespaces(get_executable_path());
  DEBUG("init_default_namespaces %d\n", sdk_version);

#if (ANDROID_VERSION_MAJOR >= 10)
  __get_tls()[TLS_SLOT_BIONIC_TLS] = __allocate_temp_bionic_tls();
#endif
}
