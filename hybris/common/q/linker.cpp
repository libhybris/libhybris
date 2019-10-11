/*
 * Copyright (C) 2008 The Android Open Source Project
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

#include <android/api-level.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/param.h>
#include <sys/vfs.h>
#include <unistd.h>

#include <new>
#include <string>
#include <unordered_map>
#include <vector>

#include "hybris_compat.h"

//#include <android-base/properties.h>
#include <android-base/scopeguard.h>

#include <async_safe/log.h>

// Private C library headers.

#include "linker.h"
#include "linker_block_allocator.h"
#include "linker_cfi.h"
#include "linker_config.h"
#include "linker_gdb_support.h"
#include "linker_globals.h"
#include "linker_debug.h"
#include "linker_dlwarning.h"
#include "linker_main.h"
#include "linker_namespaces.h"
#include "linker_sleb128.h"
#include "linker_phdr.h"
#include "linker_relocs.h"
#include "linker_reloc_iterators.h"
#include "linker_tls.h"
#include "linker_utils.h"

#include "private/bionic_globals.h"
#include "android-base/macros.h"
//#include "android-base/strings.h"
//#include "android-base/stringprintf.h"
//#include "ziparchive/zip_archive.h"


#define TMPFS_MAGIC 0x01021994

#define DF_1_PIE        0x08000000


// Override macros to use C++ style casts.
#undef ELF_ST_TYPE
#define ELF_ST_TYPE(x) (static_cast<uint32_t>(x) & 0xf)

static std::unordered_map<void*, size_t> g_dso_handle_counters;

static std::unordered_map<std::string, android_namespace_t*> g_exported_namespaces;

static LinkerTypeAllocator<soinfo> g_soinfo_allocator;
static LinkerTypeAllocator<LinkedListEntry<soinfo>> g_soinfo_links_allocator;

static LinkerTypeAllocator<android_namespace_t> g_namespace_allocator;
android_namespace_t *g_default_namespace = new (g_namespace_allocator.alloc()) android_namespace_t();
static android_namespace_t* g_anonymous_namespace = g_default_namespace;
static LinkerTypeAllocator<LinkedListEntry<android_namespace_t>> g_namespace_list_allocator;

static const char* const kLdConfigArchFilePath = "/system/etc/ld.config." ABI_STRING ".txt";

static const char* const kLdConfigFilePath = "/system/etc/ld.config.txt";
static const char* const kLdConfigVndkLiteFilePath = "/system/etc/ld.config.vndk_lite.txt";

#if defined(__LP64__)
static const char* const kSystemLibDir        = "/system/lib64";
static const char* const kOdmLibDir           = "/odm/lib64";
static const char* const kVendorLibDir        = "/vendor/lib64";
static const char* const kAsanSystemLibDir    = "/data/asan/system/lib64";
static const char* const kAsanOdmLibDir       = "/data/asan/odm/lib64";
static const char* const kAsanVendorLibDir    = "/data/asan/vendor/lib64";
static const char* const kRuntimeApexLibDir   = "/apex/com.android.runtime/lib64";
#else
static const char* const kSystemLibDir        = "/system/lib";
static const char* const kOdmLibDir           = "/odm/lib";
static const char* const kVendorLibDir        = "/vendor/lib";
static const char* const kAsanSystemLibDir    = "/data/asan/system/lib";
static const char* const kAsanOdmLibDir       = "/data/asan/odm/lib";
static const char* const kAsanVendorLibDir    = "/data/asan/vendor/lib";
static const char* const kRuntimeApexLibDir   = "/apex/com.android.runtime/lib";
#endif

static const char* const kAsanLibDirPrefix = "/data/asan";

static const char* const kDefaultLdPaths[] = {
  kSystemLibDir,
  kOdmLibDir,
  kVendorLibDir,
  nullptr
};

static const char* const kAsanDefaultLdPaths[] = {
  kAsanSystemLibDir,
  kSystemLibDir,
  kAsanOdmLibDir,
  kOdmLibDir,
  kAsanVendorLibDir,
  kVendorLibDir,
  nullptr
};

// Is ASAN enabled?
static bool g_is_asan = false;

static CFIShadowWriter g_cfi_shadow;

CFIShadowWriter* get_cfi_shadow() {
  return &g_cfi_shadow;
}

static bool is_system_library(const std::string& realpath) {
  for (const auto& dir : g_default_namespace->get_default_library_paths()) {
    if (file_is_in_dir(realpath, dir)) {
      return true;
    }
  }
  return false;
}

// Checks if the file exists and not a directory.
static bool file_exists(const char* path) {
  struct stat s;

  if (stat(path, &s) != 0) {
    return false;
  }

  return S_ISREG(s.st_mode);
}

static std::string resolve_soname(const std::string& name) {
  // We assume that soname equals to basename here

  // TODO(dimitry): consider having honest absolute-path -> soname resolution
  // note that since we might end up refusing to load this library because
  // it is not in shared libs list we need to get the soname without actually loading
  // the library.
  //
  // On the other hand there are several places where we already assume that
  // soname == basename in particular for any not-loaded library mentioned
  // in DT_NEEDED list.
  return basename(name.c_str());
}

static bool maybe_accessible_via_namespace_links(android_namespace_t* ns, const char* name) {
  std::string soname = resolve_soname(name);
  for (auto& ns_link : ns->linked_namespaces()) {
    if (ns_link.is_accessible(soname.c_str())) {
      return true;
    }
  }

  return false;
}

// TODO(dimitry): The grey-list is a workaround for http://b/26394120 ---
// gradually remove libraries from this list until it is gone.
static bool is_greylisted(android_namespace_t* ns, const char* name, const soinfo* needed_by) {
  static const char* const kLibraryGreyList[] = {
    "libandroid_runtime.so",
    "libbinder.so",
    "libcrypto.so",
    "libcutils.so",
    "libexpat.so",
    "libgui.so",
    "libmedia.so",
    "libnativehelper.so",
    "libssl.so",
    "libstagefright.so",
    "libsqlite.so",
    "libui.so",
    "libutils.so",
    "libvorbisidec.so",
    nullptr
  };

  // If you're targeting N, you don't get the greylist.
  if (g_greylist_disabled || get_application_target_sdk_version() >= __ANDROID_API_N__) {
    return false;
  }

  // if the library needed by a system library - implicitly assume it
  // is greylisted unless it is in the list of shared libraries for one or
  // more linked namespaces
  if (needed_by != nullptr && is_system_library(needed_by->get_realpath())) {
    return !maybe_accessible_via_namespace_links(ns, name);
  }

  // if this is an absolute path - make sure it points to /system/lib(64)
  if (name[0] == '/' && dirname(name) == kSystemLibDir) {
    // and reduce the path to basename
    name = basename(name);
  }

  for (size_t i = 0; kLibraryGreyList[i] != nullptr; ++i) {
    if (strcmp(name, kLibraryGreyList[i]) == 0) {
      return true;
    }
  }

  return false;
}
// END OF WORKAROUND

// Workaround for dlopen(/system/lib(64)/<soname>) when .so is in /apex. http://b/121248172
/**
 * Translate /system path to /apex path if needed
 * The workaround should work only when targetSdkVersion < Q.
 *
 * param out_name_to_apex pointing to /apex path
 * return true if translation is needed
 */
static bool translateSystemPathToApexPath(const char* name, std::string* out_name_to_apex) {
  static const char* const kSystemToRuntimeApexLibs[] = {
    "libicuuc.so",
    "libicui18n.so",
  };
  // New mapping for new apex should be added below

  // Nothing to do if target sdk version is Q or above
  if (get_application_target_sdk_version() >= __ANDROID_API_Q__) {
    return false;
  }

  // If the path isn't /system/lib, there's nothing to do.
  if (name == nullptr || dirname(name) != kSystemLibDir) {
    return false;
  }

  const char* base_name = basename(name);

  for (const char* soname : kSystemToRuntimeApexLibs) {
    if (strcmp(base_name, soname) == 0) {
      *out_name_to_apex = std::string(kRuntimeApexLibDir) + "/" + base_name;
      return true;
    }
  }

  return false;
}
// End Workaround for dlopen(/system/lib/<soname>) when .so is in /apex.

static std::vector<std::string> g_ld_preload_names;

static bool g_anonymous_namespace_initialized;

#if STATS
struct linker_stats_t {
  int count[kRelocMax];
};

static linker_stats_t linker_stats;

void count_relocation(RelocationKind kind) {
  ++linker_stats.count[kind];
}
#else
void count_relocation(RelocationKind) {
}
#endif

#if COUNT_PAGES
uint32_t bitmask[4096];
#endif

static void notify_gdb_of_load(soinfo* info) {
  if (info->is_linker() || info->is_main_executable()) {
    // gdb already knows about the linker and the main executable.
    return;
  }

  link_map* map = &(info->link_map_head);

  map->l_addr = info->load_bias;
  // link_map l_name field is not const.
  map->l_name = const_cast<char*>(info->get_realpath());
  map->l_ld = info->dynamic;

  CHECK(map->l_name != nullptr);
  CHECK(map->l_name[0] != '\0');

  notify_gdb_of_load(map);
}

static void notify_gdb_of_unload(soinfo* info) {
  notify_gdb_of_unload(&(info->link_map_head));
}

LinkedListEntry<soinfo>* SoinfoListAllocator::alloc() {
  return g_soinfo_links_allocator.alloc();
}

void SoinfoListAllocator::free(LinkedListEntry<soinfo>* entry) {
  g_soinfo_links_allocator.free(entry);
}

LinkedListEntry<android_namespace_t>* NamespaceListAllocator::alloc() {
  return g_namespace_list_allocator.alloc();
}

void NamespaceListAllocator::free(LinkedListEntry<android_namespace_t>* entry) {
  g_namespace_list_allocator.free(entry);
}

soinfo* soinfo_alloc(android_namespace_t* ns, const char* name,
                     const struct stat* file_stat, off64_t file_offset,
                     uint32_t rtld_flags) {
  if (strlen(name) >= PATH_MAX) {
    async_safe_fatal("library name \"%s\" too long", name);
  }

  TRACE("name %s: allocating soinfo for ns=%p", name, ns);

  soinfo* si = new (g_soinfo_allocator.alloc()) soinfo(ns, name, file_stat,
                                                       file_offset, rtld_flags);

  solist_add_soinfo(si);

  si->generate_handle();
  ns->add_soinfo(si);

  TRACE("name %s: allocated soinfo @ %p", name, si);
  return si;
}

static void soinfo_free(soinfo* si) {
  if (si == nullptr) {
    return;
  }

  if (si->base != 0 && si->size != 0) {
    if (!si->is_mapped_by_caller()) {
      munmap(reinterpret_cast<void*>(si->base), si->size);
    } else {
      // remap the region as PROT_NONE, MAP_ANONYMOUS | MAP_NORESERVE
      mmap(reinterpret_cast<void*>(si->base), si->size, PROT_NONE,
           MAP_FIXED | MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
    }
  }

  TRACE("name %s: freeing soinfo @ %p", si->get_realpath(), si);

  if (!solist_remove_soinfo(si)) {
    async_safe_fatal("soinfo=%p is not in soinfo_list (double unload?)", si);
  }

  // clear links to/from si
  si->remove_all_links();

  si->~soinfo();
  g_soinfo_allocator.free(si);
}

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

static bool realpath_fd(int fd, std::string* realpath) {
  std::vector<char> buf(PATH_MAX), proc_self_fd(PATH_MAX);
  snprintf(&proc_self_fd[0], proc_self_fd.size(), "/proc/self/fd/%d", fd);
  if (readlink(&proc_self_fd[0], &buf[0], buf.size()) == -1) {
    if (!is_first_stage_init()) {
      PRINT("readlink(\"%s\") failed: %s [fd=%d]", &proc_self_fd[0], strerror(errno), fd);
    }
    return false;
  }

  *realpath = &buf[0];
  return true;
}

#if defined(__arm__)

// For a given PC, find the .so that it belongs to.
// Returns the base address of the .ARM.exidx section
// for that .so, and the number of 8-byte entries
// in that section (via *pcount).
//
// Intended to be called by libc's __gnu_Unwind_Find_exidx().
_Unwind_Ptr do_dl_unwind_find_exidx(_Unwind_Ptr pc, int* pcount) {
  if (soinfo* si = find_containing_library(reinterpret_cast<void*>(pc))) {
    *pcount = si->ARM_exidx_count;
    return reinterpret_cast<_Unwind_Ptr>(si->ARM_exidx);
  }
  *pcount = 0;
  return 0;
}

#endif

// Here, we only have to provide a callback to iterate across all the
// loaded libraries. gcc_eh does the rest.
int do_dl_iterate_phdr(int (*cb)(dl_phdr_info* info, size_t size, void* data), void* data) {
  int rv = 0;
  for (soinfo* si = solist_get_head(); si != nullptr; si = si->next) {
    dl_phdr_info dl_info;
    dl_info.dlpi_addr = si->link_map_head.l_addr;
    dl_info.dlpi_name = si->link_map_head.l_name;
    dl_info.dlpi_phdr = si->phdr;
    dl_info.dlpi_phnum = si->phnum;
    rv = cb(&dl_info, sizeof(dl_phdr_info), data);
    if (rv != 0) {
      break;
    }
  }
  return rv;
}


bool soinfo_do_lookup(soinfo* si_from, const char* name, const version_info* vi,
                      soinfo** si_found_in, const soinfo_list_t& global_group,
                      const soinfo_list_t& local_group, const ElfW(Sym)** symbol) {
  SymbolName symbol_name(name);
  const ElfW(Sym)* s = nullptr;

  /* "This element's presence in a shared object library alters the dynamic linker's
   * symbol resolution algorithm for references within the library. Instead of starting
   * a symbol search with the executable file, the dynamic linker starts from the shared
   * object itself. If the shared object fails to supply the referenced symbol, the
   * dynamic linker then searches the executable file and other shared objects as usual."
   *
   * http://www.sco.com/developers/gabi/2012-12-31/ch5.dynamic.html
   *
   * Note that this is unlikely since static linker avoids generating
   * relocations for -Bsymbolic linked dynamic executables.
   */
  if (si_from->has_DT_SYMBOLIC) {
    DEBUG("%s: looking up %s in local scope (DT_SYMBOLIC)", si_from->get_realpath(), name);
    if (!si_from->find_symbol_by_name(symbol_name, vi, &s)) {
      return false;
    }

    if (s != nullptr) {
      *si_found_in = si_from;
    }
  }

  // 1. Look for it in global_group
  if (s == nullptr) {
    bool error = false;
    global_group.visit([&](soinfo* global_si) {
      DEBUG("%s: looking up %s in %s (from global group)",
          si_from->get_realpath(), name, global_si->get_realpath());
      if (!global_si->find_symbol_by_name(symbol_name, vi, &s)) {
        error = true;
        return false;
      }

      if (s != nullptr) {
        *si_found_in = global_si;
        return false;
      }

      return true;
    });

    if (error) {
      return false;
    }
  }

  // 2. Look for it in the local group
  if (s == nullptr) {
    bool error = false;
    local_group.visit([&](soinfo* local_si) {
      if (local_si == si_from && si_from->has_DT_SYMBOLIC) {
        // we already did this - skip
        return true;
      }

      DEBUG("%s: looking up %s in %s (from local group)",
          si_from->get_realpath(), name, local_si->get_realpath());
      if (!local_si->find_symbol_by_name(symbol_name, vi, &s)) {
        error = true;
        return false;
      }

      if (s != nullptr) {
        *si_found_in = local_si;
        return false;
      }

      return true;
    });

    if (error) {
      return false;
    }
  }

  if (s != nullptr) {
    TRACE_TYPE(LOOKUP, "si %s sym %s s->st_value = %p, "
               "found in %s, base = %p, load bias = %p",
               si_from->get_realpath(), name, reinterpret_cast<void*>(s->st_value),
               (*si_found_in)->get_realpath(), reinterpret_cast<void*>((*si_found_in)->base),
               reinterpret_cast<void*>((*si_found_in)->load_bias));
  }

  *symbol = s;
  return true;
}

ProtectedDataGuard::ProtectedDataGuard() {
  if (ref_count_++ == 0) {
    protect_data(PROT_READ | PROT_WRITE);
  }

  if (ref_count_ == 0) { // overflow
    async_safe_fatal("Too many nested calls to dlopen()");
  }
}

ProtectedDataGuard::~ProtectedDataGuard() {
  if (--ref_count_ == 0) {
    protect_data(PROT_READ);
  }
}

void ProtectedDataGuard::protect_data(int protection) {
  g_soinfo_allocator.protect_all(protection);
  g_soinfo_links_allocator.protect_all(protection);
  g_namespace_allocator.protect_all(protection);
  g_namespace_list_allocator.protect_all(protection);
}

size_t ProtectedDataGuard::ref_count_ = 0;

// Each size has it's own allocator.
template<size_t size>
class SizeBasedAllocator {
 public:
  static void* alloc() {
    return allocator_.alloc();
  }

  static void free(void* ptr) {
    allocator_.free(ptr);
  }

  static void purge() {
    allocator_.purge();
  }

 private:
  static LinkerBlockAllocator allocator_;
};

template<size_t size>
LinkerBlockAllocator SizeBasedAllocator<size>::allocator_(size);

template<typename T>
class TypeBasedAllocator {
 public:
  static T* alloc() {
    return reinterpret_cast<T*>(SizeBasedAllocator<sizeof(T)>::alloc());
  }

  static void free(T* ptr) {
    SizeBasedAllocator<sizeof(T)>::free(ptr);
  }

  static void purge() {
    SizeBasedAllocator<sizeof(T)>::purge();
  }
};

class LoadTask {
 public:
  struct deleter_t {
    void operator()(LoadTask* t) {
      t->~LoadTask();
      TypeBasedAllocator<LoadTask>::free(t);
    }
  };

  static deleter_t deleter;

  static LoadTask* create(const char* name,
                          soinfo* needed_by,
                          android_namespace_t* start_from,
                          std::unordered_map<const soinfo*, ElfReader>* readers_map) {
    LoadTask* ptr = TypeBasedAllocator<LoadTask>::alloc();
    return new (ptr) LoadTask(name, needed_by, start_from, readers_map);
  }

  const char* get_name() const {
    return name_;
  }

  soinfo* get_needed_by() const {
    return needed_by_;
  }

  soinfo* get_soinfo() const {
    return si_;
  }

  void set_soinfo(soinfo* si) {
    si_ = si;
  }

  off64_t get_file_offset() const {
    return file_offset_;
  }

  void set_file_offset(off64_t offset) {
    file_offset_ = offset;
  }

  int get_fd() const {
    return fd_;
  }

  void set_fd(int fd, bool assume_ownership) {
    if (fd_ != -1 && close_fd_) {
      close(fd_);
    }
    fd_ = fd;
    close_fd_ = assume_ownership;
  }

  const android_dlextinfo* get_extinfo() const {
    return extinfo_;
  }

  void set_extinfo(const android_dlextinfo* extinfo) {
    extinfo_ = extinfo;
  }

  bool is_dt_needed() const {
    return is_dt_needed_;
  }

  void set_dt_needed(bool is_dt_needed) {
    is_dt_needed_ = is_dt_needed;
  }

  // returns the namespace from where we need to start loading this.
  const android_namespace_t* get_start_from() const {
    return start_from_;
  }

  const ElfReader& get_elf_reader() const {
    CHECK(si_ != nullptr);
    return (*elf_readers_map_)[si_];
  }

  ElfReader& get_elf_reader() {
    CHECK(si_ != nullptr);
    return (*elf_readers_map_)[si_];
  }

  std::unordered_map<const soinfo*, ElfReader>* get_readers_map() {
    return elf_readers_map_;
  }

  bool read(const char* realpath, off64_t file_size) {
    ElfReader& elf_reader = get_elf_reader();
    return elf_reader.Read(realpath, fd_, file_offset_, file_size);
  }

  bool load(address_space_params* address_space) {
    ElfReader& elf_reader = get_elf_reader();
    if (!elf_reader.Load(address_space)) {
      return false;
    }

    si_->base = elf_reader.load_start();
    si_->size = elf_reader.load_size();
    si_->set_mapped_by_caller(elf_reader.is_mapped_by_caller());
    si_->load_bias = elf_reader.load_bias();
    si_->phnum = elf_reader.phdr_count();
    si_->phdr = elf_reader.loaded_phdr();

    return true;
  }

 private:
  LoadTask(const char* name,
           soinfo* needed_by,
           android_namespace_t* start_from,
           std::unordered_map<const soinfo*, ElfReader>* readers_map)
    : name_(name), needed_by_(needed_by), si_(nullptr),
      fd_(-1), close_fd_(false), file_offset_(0), elf_readers_map_(readers_map),
      is_dt_needed_(false), start_from_(start_from) {}

  ~LoadTask() {
    if (fd_ != -1 && close_fd_) {
      close(fd_);
    }
  }

  const char* name_;
  soinfo* needed_by_;
  soinfo* si_;
  const android_dlextinfo* extinfo_;
  int fd_;
  bool close_fd_;
  off64_t file_offset_;
  std::unordered_map<const soinfo*, ElfReader>* elf_readers_map_;
  // TODO(dimitry): needed by workaround for http://b/26394120 (the grey-list)
  bool is_dt_needed_;
  // END OF WORKAROUND
  const android_namespace_t* const start_from_;

  DISALLOW_IMPLICIT_CONSTRUCTORS(LoadTask);
};

LoadTask::deleter_t LoadTask::deleter;

template <typename T>
using linked_list_t = LinkedList<T, TypeBasedAllocator<LinkedListEntry<T>>>;

typedef linked_list_t<soinfo> SoinfoLinkedList;
typedef linked_list_t<const char> StringLinkedList;
typedef std::vector<LoadTask*> LoadTaskList;

enum walk_action_result_t : uint32_t {
  kWalkStop = 0,
  kWalkContinue = 1,
  kWalkSkip = 2
};

// This function walks down the tree of soinfo dependencies
// in breadth-first order and
//   * calls action(soinfo* si) for each node, and
//   * terminates walk if action returns kWalkStop
//   * skips children of the node if action
//     return kWalkSkip
//
// walk_dependencies_tree returns false if walk was terminated
// by the action and true otherwise.
template<typename F>
static bool walk_dependencies_tree(soinfo* root_soinfo, F action) {
  SoinfoLinkedList visit_list;
  SoinfoLinkedList visited;

  visit_list.push_back(root_soinfo);

  soinfo* si;
  while ((si = visit_list.pop_front()) != nullptr) {
    if (visited.contains(si)) {
      continue;
    }

    walk_action_result_t result = action(si);

    if (result == kWalkStop) {
      return false;
    }

    visited.push_back(si);

    if (result != kWalkSkip) {
      si->get_children().for_each([&](soinfo* child) {
        visit_list.push_back(child);
      });
    }
  }

  return true;
}


static const ElfW(Sym)* dlsym_handle_lookup(android_namespace_t* ns,
                                            soinfo* root,
                                            soinfo* skip_until,
                                            soinfo** found,
                                            SymbolName& symbol_name,
                                            const version_info* vi) {
  const ElfW(Sym)* result = nullptr;
  bool skip_lookup = skip_until != nullptr;

  walk_dependencies_tree(root, [&](soinfo* current_soinfo) {
    if (skip_lookup) {
      skip_lookup = current_soinfo != skip_until;
      return kWalkContinue;
    }

    if (!ns->is_accessible(current_soinfo)) {
      return kWalkSkip;
    }

    if (!current_soinfo->find_symbol_by_name(symbol_name, vi, &result)) {
      result = nullptr;
      return kWalkStop;
    }

    if (result != nullptr) {
      *found = current_soinfo;
      return kWalkStop;
    }

    return kWalkContinue;
  });

  return result;
}

static const ElfW(Sym)* dlsym_linear_lookup(android_namespace_t* ns,
                                            const char* name,
                                            const version_info* vi,
                                            soinfo** found,
                                            soinfo* caller,
                                            void* handle);

// This is used by dlsym(3).  It performs symbol lookup only within the
// specified soinfo object and its dependencies in breadth first order.
static const ElfW(Sym)* dlsym_handle_lookup(soinfo* si,
                                            soinfo** found,
                                            const char* name,
                                            const version_info* vi) {
  // According to man dlopen(3) and posix docs in the case when si is handle
  // of the main executable we need to search not only in the executable and its
  // dependencies but also in all libraries loaded with RTLD_GLOBAL.
  //
  // Since RTLD_GLOBAL is always set for the main executable and all dt_needed shared
  // libraries and they are loaded in breath-first (correct) order we can just execute
  // dlsym(RTLD_DEFAULT, ...); instead of doing two stage lookup.
  if (si == solist_get_somain()) {
    return dlsym_linear_lookup(g_default_namespace, name, vi, found, nullptr, RTLD_DEFAULT);
  }

  SymbolName symbol_name(name);
  // note that the namespace is not the namespace associated with caller_addr
  // we use ns associated with root si intentionally here. Using caller_ns
  // causes problems when user uses dlopen_ext to open a library in the separate
  // namespace and then calls dlsym() on the handle.
  return dlsym_handle_lookup(si->get_primary_namespace(), si, nullptr, found, symbol_name, vi);
}

/* This is used by dlsym(3) to performs a global symbol lookup. If the
   start value is null (for RTLD_DEFAULT), the search starts at the
   beginning of the global solist. Otherwise the search starts at the
   specified soinfo (for RTLD_NEXT).
 */
static const ElfW(Sym)* dlsym_linear_lookup(android_namespace_t* ns,
                                            const char* name,
                                            const version_info* vi,
                                            soinfo** found,
                                            soinfo* caller,
                                            void* handle) {
  SymbolName symbol_name(name);

  auto& soinfo_list = ns->soinfo_list();
  auto start = soinfo_list.begin();

  if (handle == RTLD_NEXT) {
    if (caller == nullptr) {
      return nullptr;
    } else {
      auto it = soinfo_list.find(caller);
      CHECK (it != soinfo_list.end());
      start = ++it;
    }
  }

  const ElfW(Sym)* s = nullptr;
  for (auto it = start, end = soinfo_list.end(); it != end; ++it) {
    soinfo* si = *it;
    // Do not skip RTLD_LOCAL libraries in dlsym(RTLD_DEFAULT, ...)
    // if the library is opened by application with target api level < M.
    // See http://b/21565766
    if ((si->get_rtld_flags() & RTLD_GLOBAL) == 0 &&
        si->get_target_sdk_version() >= __ANDROID_API_M__) {
      continue;
    }

    if (!si->find_symbol_by_name(symbol_name, vi, &s)) {
      return nullptr;
    }

    if (s != nullptr) {
      *found = si;
      break;
    }
  }

  // If not found - use dlsym_handle_lookup for caller's local_group
  if (s == nullptr && caller != nullptr) {
    soinfo* local_group_root = caller->get_local_group_root();

    return dlsym_handle_lookup(local_group_root->get_primary_namespace(),
                               local_group_root,
                               (handle == RTLD_NEXT) ? caller : nullptr,
                               found,
                               symbol_name,
                               vi);
  }

  if (s != nullptr) {
    TRACE_TYPE(LOOKUP, "%s s->st_value = %p, found->base = %p",
               name, reinterpret_cast<void*>(s->st_value), reinterpret_cast<void*>((*found)->base));
  }

  return s;
}

soinfo* find_containing_library(const void* p) {
  ElfW(Addr) address = reinterpret_cast<ElfW(Addr)>(p);
  for (soinfo* si = solist_get_head(); si != nullptr; si = si->next) {
    if (address < si->base || address - si->base >= si->size) {
      continue;
    }
    ElfW(Addr) vaddr = address - si->load_bias;
    for (size_t i = 0; i != si->phnum; ++i) {
      const ElfW(Phdr)* phdr = &si->phdr[i];
      if (phdr->p_type != PT_LOAD) {
        continue;
      }
      if (vaddr >= phdr->p_vaddr && vaddr < phdr->p_vaddr + phdr->p_memsz) {
        return si;
      }
    }
  }
  return nullptr;
}

class ZipArchiveCache {
 public:
  ZipArchiveCache() {}
  ~ZipArchiveCache() {};

  //bool get_or_open(const char* zip_path, ZipArchiveHandle* handle);
 private:
  DISALLOW_COPY_AND_ASSIGN(ZipArchiveCache);

 // std::unordered_map<std::string, ZipArchiveHandle> cache_;
};
/*
bool ZipArchiveCache::get_or_open(const char* zip_path, ZipArchiveHandle* handle) {
  std::string key(zip_path);

  auto it = cache_.find(key);
  if (it != cache_.end()) {
    *handle = it->second;
    return true;
  }

  int fd = TEMP_FAILURE_RETRY(open(zip_path, O_RDONLY | O_CLOEXEC));
  if (fd == -1) {
    return false;
  }

  if (OpenArchiveFd(fd, "", handle) != 0) {
    // invalid zip-file (?)
    CloseArchive(*handle);
    return false;
  }

  cache_[key] = *handle;
  return true;
}

ZipArchiveCache::~ZipArchiveCache() {
  for (const auto& it : cache_) {
    CloseArchive(it.second);
  }
}

static int open_library_in_zipfile(ZipArchiveCache* zip_archive_cache,
                                   const char* const input_path,
                                   off64_t* file_offset, std::string* realpath) {
  std::string normalized_path;
  if (!normalize_path(input_path, &normalized_path)) {
    return -1;
  }

  const char* const path = normalized_path.c_str();
  TRACE("Trying zip file open from path \"%s\" -> normalized \"%s\"", input_path, path);

  // Treat an '!/' separator inside a path as the separator between the name
  // of the zip file on disk and the subdirectory to search within it.
  // For example, if path is "foo.zip!/bar/bas/x.so", then we search for
  // "bar/bas/x.so" within "foo.zip".
  const char* const separator = strstr(path, kZipFileSeparator);
  if (separator == nullptr) {
    return -1;
  }

  char buf[512];
  if (strlcpy(buf, path, sizeof(buf)) >= sizeof(buf)) {
    PRINT("Warning: ignoring very long library path: %s", path);
    return -1;
  }

  buf[separator - path] = '\0';

  const char* zip_path = buf;
  const char* file_path = &buf[separator - path + 2];
  int fd = TEMP_FAILURE_RETRY(open(zip_path, O_RDONLY | O_CLOEXEC));
  if (fd == -1) {
    return -1;
  }

  ZipArchiveHandle handle;
  if (!zip_archive_cache->get_or_open(zip_path, &handle)) {
    // invalid zip-file (?)
    close(fd);
    return -1;
  }

  ZipEntry entry;

  if (FindEntry(handle, ZipString(file_path), &entry) != 0) {
    // Entry was not found.
    close(fd);
    return -1;
  }

  // Check if it is properly stored
  if (entry.method != kCompressStored || (entry.offset % PAGE_SIZE) != 0) {
    close(fd);
    return -1;
  }

  *file_offset = entry.offset;

  if (realpath_fd(fd, realpath)) {
    *realpath += separator;
  } else {
    if (!is_first_stage_init()) {
      PRINT("warning: unable to get realpath for the library \"%s\". Will use given path.",
            normalized_path.c_str());
    }
    *realpath = normalized_path;
  }

  return fd;
}
*/
static bool format_path(char* buf, size_t buf_size, const char* path, const char* name) {
  int n = snprintf(buf, buf_size, "%s/%s", path, name);
  if (n < 0 || n >= static_cast<int>(buf_size)) {
    PRINT("Warning: ignoring very long library path: %s/%s", path, name);
    return false;
  }

  return true;
}

static int open_library_at_path(ZipArchiveCache* zip_archive_cache,
                                const char* path, off64_t* file_offset,
                                std::string* realpath) {
  int fd = -1;
  /*
  if (strstr(path, kZipFileSeparator) != nullptr) {
    fd = open_library_in_zipfile(zip_archive_cache, path, file_offset, realpath);
  }
*/
  if (fd == -1) {
    fd = TEMP_FAILURE_RETRY(open(path, O_RDONLY | O_CLOEXEC));
    if (fd != -1) {
      *file_offset = 0;
      if (!realpath_fd(fd, realpath)) {
        if (!is_first_stage_init()) {
          PRINT("warning: unable to get realpath for the library \"%s\". Will use given path.",
                path);
        }
        *realpath = path;
      }
    }
  }

  return fd;
}

static int open_library_on_paths(ZipArchiveCache* zip_archive_cache,
                                 const char* name, off64_t* file_offset,
                                 const std::vector<std::string>& paths,
                                 std::string* realpath) {
  for (const auto& path : paths) {
    char buf[512];
    if (!format_path(buf, sizeof(buf), path.c_str(), name)) {
      continue;
    }

    int fd = open_library_at_path(zip_archive_cache, buf, file_offset, realpath);
    if (fd != -1) {
      return fd;
    }
  }

  return -1;
}

static int open_library(android_namespace_t* ns,
                        ZipArchiveCache* zip_archive_cache,
                        const char* name, soinfo *needed_by,
                        off64_t* file_offset, std::string* realpath) {
  TRACE("[ opening %s from namespace %s ]", name, ns->get_name());

  // If the name contains a slash, we should attempt to open it directly and not search the paths.
  if (strchr(name, '/') != nullptr) {
    int fd = -1;

    /*if (strstr(name, kZipFileSeparator) != nullptr) {
      fd = open_library_in_zipfile(zip_archive_cache, name, file_offset, realpath);
    }*/

    if (fd == -1) {
      fd = TEMP_FAILURE_RETRY(open(name, O_RDONLY | O_CLOEXEC));
      if (fd != -1) {
        *file_offset = 0;
        if (!realpath_fd(fd, realpath)) {
          if (!is_first_stage_init()) {
            PRINT("warning: unable to get realpath for the library \"%s\". Will use given path.",
                  name);
          }
          *realpath = name;
        }
      }
    }

    return fd;
  }

  // Otherwise we try LD_LIBRARY_PATH first, and fall back to the default library path
//  TRACE("[ opening %s from namespace %s from %s]", name, ns->get_name(),ns->get_ld_library_paths());
  int fd = open_library_on_paths(zip_archive_cache, name, file_offset, ns->get_ld_library_paths(), realpath);
  if (fd == -1 && needed_by != nullptr) {
//    TRACE("[ opening %s from namespace %s from %s]", name, ns->get_name(),needed_by->get_dt_runpath());
    fd = open_library_on_paths(zip_archive_cache, name, file_offset, needed_by->get_dt_runpath(), realpath);
    // Check if the library is accessible
    if (fd != -1 && !ns->is_accessible(*realpath)) {
      close(fd);
      fd = -1;
    }
  }

  if (fd == -1) {
    //TRACE("[ opening %s from namespace %s from %s]", name, ns->get_name(),ns->get_default_library_paths());
    fd = open_library_on_paths(zip_archive_cache, name, file_offset, ns->get_default_library_paths(), realpath);
  }

  return fd;
}

int open_executable(const char* path, off64_t* file_offset, std::string* realpath) {
  ZipArchiveCache zip_archive_cache;
  return open_library_at_path(&zip_archive_cache, path, file_offset, realpath);
}

const char* fix_dt_needed(const char* dt_needed, const char* sopath __unused) {
#if !defined(__LP64__)
  // Work around incorrect DT_NEEDED entries for old apps: http://b/21364029
  int app_target_api_level = get_application_target_sdk_version();
  if (app_target_api_level < __ANDROID_API_M__) {
    const char* bname = basename(dt_needed);
    if (bname != dt_needed) {
      DL_WARN_documented_change(__ANDROID_API_M__,
                                "invalid-dt_needed-entries-enforced-for-api-level-23",
                                "library \"%s\" has invalid DT_NEEDED entry \"%s\"",
                                sopath, dt_needed, app_target_api_level);
      add_dlwarning(sopath, "invalid DT_NEEDED entry",  dt_needed);
    }

    return bname;
  }
#endif
  return dt_needed;
}

template<typename F>
static void for_each_dt_needed(const ElfReader& elf_reader, F action) {
  for (const ElfW(Dyn)* d = elf_reader.dynamic(); d->d_tag != DT_NULL; ++d) {
    if (d->d_tag == DT_NEEDED) {
      action(fix_dt_needed(elf_reader.get_string(d->d_un.d_val), elf_reader.name()));
    }
  }
}

static bool find_loaded_library_by_inode(android_namespace_t* ns,
                                         const struct stat& file_stat,
                                         off64_t file_offset,
                                         bool search_linked_namespaces,
                                         soinfo** candidate) {

  auto predicate = [&](soinfo* si) {
    return si->get_st_dev() != 0 &&
           si->get_st_ino() != 0 &&
           si->get_st_dev() == file_stat.st_dev &&
           si->get_st_ino() == file_stat.st_ino &&
           si->get_file_offset() == file_offset;
  };

  *candidate = ns->soinfo_list().find_if(predicate);

  if (*candidate == nullptr && search_linked_namespaces) {
    for (auto& link : ns->linked_namespaces()) {
      android_namespace_t* linked_ns = link.linked_namespace();
      soinfo* si = linked_ns->soinfo_list().find_if(predicate);

      if (si != nullptr && link.is_accessible(si->get_soname())) {
        *candidate = si;
        return true;
      }
    }
  }

  return *candidate != nullptr;
}

static bool find_loaded_library_by_realpath(android_namespace_t* ns, const char* realpath,
                                            bool search_linked_namespaces, soinfo** candidate) {
  auto predicate = [&](soinfo* si) { return strcmp(realpath, si->get_realpath()) == 0; };

  *candidate = ns->soinfo_list().find_if(predicate);

  if (*candidate == nullptr && search_linked_namespaces) {
    for (auto& link : ns->linked_namespaces()) {
      android_namespace_t* linked_ns = link.linked_namespace();
      soinfo* si = linked_ns->soinfo_list().find_if(predicate);

      if (si != nullptr && link.is_accessible(si->get_soname())) {
        *candidate = si;
        return true;
      }
    }
  }

  return *candidate != nullptr;
}

static bool load_library(android_namespace_t* ns,
                         LoadTask* task,
                         LoadTaskList* load_tasks,
                         int rtld_flags,
                         const std::string& realpath,
                         bool search_linked_namespaces) {
  off64_t file_offset = task->get_file_offset();
  const char* name = task->get_name();
  const android_dlextinfo* extinfo = task->get_extinfo();

  LD_LOG(kLogDlopen, "load_library(ns=%s, task=%s, flags=0x%x, realpath=%s)", ns->get_name(), name,
         rtld_flags, realpath.c_str());

  if ((file_offset % PAGE_SIZE) != 0) {
    DL_ERR("file offset for the library \"%s\" is not page-aligned: %" PRId64, name, file_offset);
    return false;
  }
  if (file_offset < 0) {
    DL_ERR("file offset for the library \"%s\" is negative: %" PRId64, name, file_offset);
    return false;
  }

  struct stat file_stat;
  if (TEMP_FAILURE_RETRY(fstat(task->get_fd(), &file_stat)) != 0) {
    DL_ERR("unable to stat file for the library \"%s\": %s", name, strerror(errno));
    return false;
  }
  if (file_offset >= file_stat.st_size) {
    DL_ERR("file offset for the library \"%s\" >= file size: %" PRId64 " >= %" PRId64,
        name, file_offset, file_stat.st_size);
    return false;
  }

  // Check for symlink and other situations where
  // file can have different names, unless ANDROID_DLEXT_FORCE_LOAD is set
  if (extinfo == nullptr || (extinfo->flags & ANDROID_DLEXT_FORCE_LOAD) == 0) {
    soinfo* si = nullptr;
    if (find_loaded_library_by_inode(ns, file_stat, file_offset, search_linked_namespaces, &si)) {
      LD_LOG(kLogDlopen,
             "load_library(ns=%s, task=%s): Already loaded under different name/path \"%s\" - "
             "will return existing soinfo",
             ns->get_name(), name, si->get_realpath());
      task->set_soinfo(si);
      return true;
    }
  }

  if ((rtld_flags & RTLD_NOLOAD) != 0) {
    DL_ERR("library \"%s\" wasn't loaded and RTLD_NOLOAD prevented it", name);
    return false;
  }

  struct statfs fs_stat;
  if (TEMP_FAILURE_RETRY(fstatfs(task->get_fd(), &fs_stat)) != 0) {
    DL_ERR("unable to fstatfs file for the library \"%s\": %s", name, strerror(errno));
    return false;
  }

  // do not check accessibility using realpath if fd is located on tmpfs
  // this enables use of memfd_create() for apps
  if ((fs_stat.f_type != TMPFS_MAGIC) && (!ns->is_accessible(realpath))) {
    // TODO(dimitry): workaround for http://b/26394120 - the grey-list

    // TODO(dimitry) before O release: add a namespace attribute to have this enabled
    // only for classloader-namespaces
    const soinfo* needed_by = task->is_dt_needed() ? task->get_needed_by() : nullptr;
    if (is_greylisted(ns, name, needed_by)) {
      // print warning only if needed by non-system library
      if (needed_by == nullptr || !is_system_library(needed_by->get_realpath())) {
        const soinfo* needed_or_dlopened_by = task->get_needed_by();
        const char* sopath = needed_or_dlopened_by == nullptr ? "(unknown)" :
                                                      needed_or_dlopened_by->get_realpath();
        DL_WARN_documented_change(__ANDROID_API_N__,
                                  "private-api-enforced-for-api-level-24",
                                  "library \"%s\" (\"%s\") needed or dlopened by \"%s\" "
                                  "is not accessible by namespace \"%s\"",
                                  name, realpath.c_str(), sopath, ns->get_name());
        add_dlwarning(sopath, "unauthorized access to",  name);
      }
    } else {
      // do not load libraries if they are not accessible for the specified namespace.
      const char* needed_or_dlopened_by = task->get_needed_by() == nullptr ?
                                          "(unknown)" :
                                          task->get_needed_by()->get_realpath();

      DL_ERR("library \"%s\" needed or dlopened by \"%s\" is not accessible for the namespace \"%s\"",
             name, needed_or_dlopened_by, ns->get_name());

      // do not print this if a library is in the list of shared libraries for linked namespaces
      if (!maybe_accessible_via_namespace_links(ns, name)) {
        PRINT("library \"%s\" (\"%s\") needed or dlopened by \"%s\" is not accessible for the"
              " namespace: [name=\"%s\", ld_library_paths=\"%s\", default_library_paths=\"%s\","
              " permitted_paths=\"%s\"]",
              name, realpath.c_str(),
              needed_or_dlopened_by,
              ns->get_name(),
              join(ns->get_ld_library_paths(), ':').c_str(),
              join(ns->get_default_library_paths(), ':').c_str(),
              join(ns->get_permitted_paths(), ':').c_str());
      }
      return false;
    }
  }

  soinfo* si = soinfo_alloc(ns, realpath.c_str(), &file_stat, file_offset, rtld_flags);
  if (si == nullptr) {
    return false;
  }

  task->set_soinfo(si);

  // Read the ELF header and some of the segments.
  if (!task->read(realpath.c_str(), file_stat.st_size)) {
    soinfo_free(si);
    task->set_soinfo(nullptr);
    return false;
  }

  // find and set DT_RUNPATH and dt_soname
  // Note that these field values are temporary and are
  // going to be overwritten on soinfo::prelink_image
  // with values from PT_LOAD segments.
  const ElfReader& elf_reader = task->get_elf_reader();
  for (const ElfW(Dyn)* d = elf_reader.dynamic(); d->d_tag != DT_NULL; ++d) {
    if (d->d_tag == DT_RUNPATH) {
      si->set_dt_runpath(elf_reader.get_string(d->d_un.d_val));
    }
    if (d->d_tag == DT_SONAME) {
      si->set_soname(elf_reader.get_string(d->d_un.d_val));
    }
  }

#if !defined(__ANDROID__)
  // Bionic on the host currently uses some Android prebuilts, which don't set
  // DT_RUNPATH with any relative paths, so they can't find their dependencies.
  // b/118058804
  if (si->get_dt_runpath().empty()) {
    si->set_dt_runpath("$ORIGIN/../lib64:$ORIGIN/lib64");
  }
#endif

  for_each_dt_needed(task->get_elf_reader(), [&](const char* name) {
    LD_LOG(kLogDlopen, "load_library(ns=%s, task=%s): Adding DT_NEEDED task: %s",
           ns->get_name(), task->get_name(), name);
    load_tasks->push_back(LoadTask::create(name, si, ns, task->get_readers_map()));
  });

  return true;
}

static bool load_library(android_namespace_t* ns,
                         LoadTask* task,
                         ZipArchiveCache* zip_archive_cache,
                         LoadTaskList* load_tasks,
                         int rtld_flags,
                         bool search_linked_namespaces) {
  const char* name = task->get_name();
  soinfo* needed_by = task->get_needed_by();
  const android_dlextinfo* extinfo = task->get_extinfo();

  off64_t file_offset;
  std::string realpath;
  if (extinfo != nullptr && (extinfo->flags & ANDROID_DLEXT_USE_LIBRARY_FD) != 0) {
    file_offset = 0;
    if ((extinfo->flags & ANDROID_DLEXT_USE_LIBRARY_FD_OFFSET) != 0) {
      file_offset = extinfo->library_fd_offset;
    }

    if (!realpath_fd(extinfo->library_fd, &realpath)) {
      if (!is_first_stage_init()) {
        PRINT(
            "warning: unable to get realpath for the library \"%s\" by extinfo->library_fd. "
            "Will use given name.",
            name);
      }
      realpath = name;
    }

    task->set_fd(extinfo->library_fd, false);
    task->set_file_offset(file_offset);
    return load_library(ns, task, load_tasks, rtld_flags, realpath, search_linked_namespaces);
  }

  // Open the file.
  int fd = open_library(ns, zip_archive_cache, name, needed_by, &file_offset, &realpath);
  if (fd == -1) {
    DL_ERR("library \"%s\" not found", name);
    return false;
  }

  task->set_fd(fd, true);
  task->set_file_offset(file_offset);

  return load_library(ns, task, load_tasks, rtld_flags, realpath, search_linked_namespaces);
}

static bool find_loaded_library_by_soname(android_namespace_t* ns,
                                          const char* name,
                                          soinfo** candidate) {
  return !ns->soinfo_list().visit([&](soinfo* si) {
    const char* soname = si->get_soname();
    if (soname != nullptr && (strcmp(name, soname) == 0)) {
      *candidate = si;
      return false;
    }

    return true;
  });
}

// Returns true if library was found and false otherwise
static bool find_loaded_library_by_soname(android_namespace_t* ns,
                                         const char* name,
                                         bool search_linked_namespaces,
                                         soinfo** candidate) {
  *candidate = nullptr;

  // Ignore filename with path.
  if (strchr(name, '/') != nullptr) {
    return false;
  }

  bool found = find_loaded_library_by_soname(ns, name, candidate);

  if (!found && search_linked_namespaces) {
    // if a library was not found - look into linked namespaces
    for (auto& link : ns->linked_namespaces()) {
      if (!link.is_accessible(name)) {
        continue;
      }

      android_namespace_t* linked_ns = link.linked_namespace();

      if (find_loaded_library_by_soname(linked_ns, name, candidate)) {
        return true;
      }
    }
  }

  return found;
}

static bool find_library_in_linked_namespace(const android_namespace_link_t& namespace_link,
                                             LoadTask* task) {
  android_namespace_t* ns = namespace_link.linked_namespace();

  soinfo* candidate;
  bool loaded = false;

  std::string soname;
  if (find_loaded_library_by_soname(ns, task->get_name(), false, &candidate)) {
    loaded = true;
    soname = candidate->get_soname();
  } else {
    soname = resolve_soname(task->get_name());
  }

  if (!namespace_link.is_accessible(soname.c_str())) {
    // the library is not accessible via namespace_link
    LD_LOG(kLogDlopen,
           "find_library_in_linked_namespace(ns=%s, task=%s): Not accessible (soname=%s)",
           ns->get_name(), task->get_name(), soname.c_str());
    return false;
  }

  // if library is already loaded - return it
  if (loaded) {
    LD_LOG(kLogDlopen, "find_library_in_linked_namespace(ns=%s, task=%s): Already loaded",
           ns->get_name(), task->get_name());
    task->set_soinfo(candidate);
    return true;
  }

  // returning true with empty soinfo means that the library is okay to be
  // loaded in the namespace but has not yet been loaded there before.
  LD_LOG(kLogDlopen, "find_library_in_linked_namespace(ns=%s, task=%s): Ok to load", ns->get_name(),
         task->get_name());
  task->set_soinfo(nullptr);
  return true;
}

static bool find_library_internal(android_namespace_t* ns,
                                  LoadTask* task,
                                  ZipArchiveCache* zip_archive_cache,
                                  LoadTaskList* load_tasks,
                                  int rtld_flags,
                                  bool search_linked_namespaces) {
  soinfo* candidate;

  if (find_loaded_library_by_soname(ns, task->get_name(), search_linked_namespaces, &candidate)) {
    LD_LOG(kLogDlopen,
           "find_library_internal(ns=%s, task=%s): Already loaded (by soname): %s",
           ns->get_name(), task->get_name(), candidate->get_realpath());
    task->set_soinfo(candidate);
    return true;
  }

  // Library might still be loaded, the accurate detection
  // of this fact is done by load_library.
  TRACE("[ \"%s\" find_loaded_library_by_soname failed (*candidate=%s@%p). Trying harder... ]",
        task->get_name(), candidate == nullptr ? "n/a" : candidate->get_realpath(), candidate);

  if (load_library(ns, task, zip_archive_cache, load_tasks, rtld_flags, search_linked_namespaces)) {
    return true;
  }

  // TODO(dimitry): workaround for http://b/26394120 (the grey-list)
  if (ns->is_greylist_enabled() && is_greylisted(ns, task->get_name(), task->get_needed_by())) {
    // For the libs in the greylist, switch to the default namespace and then
    // try the load again from there. The library could be loaded from the
    // default namespace or from another namespace (e.g. runtime) that is linked
    // from the default namespace.
    LD_LOG(kLogDlopen,
           "find_library_internal(ns=%s, task=%s): Greylisted library - trying namespace %s",
           ns->get_name(), task->get_name(), g_default_namespace->get_name());
    ns = g_default_namespace;
    if (load_library(ns, task, zip_archive_cache, load_tasks, rtld_flags,
                     search_linked_namespaces)) {
      return true;
    }
  }
  // END OF WORKAROUND

  if (search_linked_namespaces) {
    // if a library was not found - look into linked namespaces
    // preserve current dlerror in the case it fails.
    DlErrorRestorer dlerror_restorer;
    LD_LOG(kLogDlopen, "find_library_internal(ns=%s, task=%s): Trying %zu linked namespaces",
           ns->get_name(), task->get_name(), ns->linked_namespaces().size());
    for (auto& linked_namespace : ns->linked_namespaces()) {
      if (find_library_in_linked_namespace(linked_namespace, task)) {
        if (task->get_soinfo() == nullptr) {
          // try to load the library - once namespace boundary is crossed
          // we need to load a library within separate load_group
          // to avoid using symbols from foreign namespace while.
          //
          // However, actual linking is deferred until when the global group
          // is fully identified and is applied to all namespaces.
          // Otherwise, the libs in the linked namespace won't get symbols from
          // the global group.
          if (load_library(linked_namespace.linked_namespace(), task, zip_archive_cache, load_tasks, rtld_flags, false)) {
            LD_LOG(
                kLogDlopen, "find_library_internal(ns=%s, task=%s): Found in linked namespace %s",
                ns->get_name(), task->get_name(), linked_namespace.linked_namespace()->get_name());
            return true;
          }
        } else {
          // lib is already loaded
          return true;
        }
      }
    }
  }

  return false;
}

static void soinfo_unload(soinfo* si);

static void shuffle(std::vector<LoadTask*>* v) {
  if (is_first_stage_init()) {
    // arc4random* is not available in first stage init because /dev/random
    // hasn't yet been created.
    return;
  }
  for (size_t i = 0, size = v->size(); i < size; ++i) {
    size_t n = size - i;
    //size_t r = arc4random_uniform(n);
    size_t r = rand() % n;
    std::swap((*v)[n-1], (*v)[r]);
  }
}


// add_as_children - add first-level loaded libraries (i.e. library_names[], but
// not their transitive dependencies) as children of the start_with library.
// This is false when find_libraries is called for dlopen(), when newly loaded
// libraries must form a disjoint tree.
bool find_libraries(android_namespace_t* ns,
                    soinfo* start_with,
                    const char* const library_names[],
                    size_t library_names_count,
                    soinfo* soinfos[],
                    std::vector<soinfo*>* ld_preloads,
                    size_t ld_preloads_count,
                    int rtld_flags,
                    const android_dlextinfo* extinfo,
                    bool add_as_children,
                    bool search_linked_namespaces,
                    std::vector<android_namespace_t*>* namespaces) {
  // Step 0: prepare.
  std::unordered_map<const soinfo*, ElfReader> readers_map;
  LoadTaskList load_tasks;

  for (size_t i = 0; i < library_names_count; ++i) {
    const char* name = library_names[i];
    load_tasks.push_back(LoadTask::create(name, start_with, ns, &readers_map));
  }

  // If soinfos array is null allocate one on stack.
  // The array is needed in case of failure; for example
  // when library_names[] = {libone.so, libtwo.so} and libone.so
  // is loaded correctly but libtwo.so failed for some reason.
  // In this case libone.so should be unloaded on return.
  // See also implementation of failure_guard below.

  if (soinfos == nullptr) {
    size_t soinfos_size = sizeof(soinfo*)*library_names_count;
    soinfos = reinterpret_cast<soinfo**>(alloca(soinfos_size));
    memset(soinfos, 0, soinfos_size);
  }

  // list of libraries to link - see step 2.
  size_t soinfos_count = 0;

  auto scope_guard = android::base::make_scope_guard([&]() {
    for (LoadTask* t : load_tasks) {
      LoadTask::deleter(t);
    }
  });

  ZipArchiveCache zip_archive_cache;

  // Step 1: expand the list of load_tasks to include
  // all DT_NEEDED libraries (do not load them just yet)
  for (size_t i = 0; i<load_tasks.size(); ++i) {
    LoadTask* task = load_tasks[i];
    soinfo* needed_by = task->get_needed_by();

    bool is_dt_needed = needed_by != nullptr && (needed_by != start_with || add_as_children);
    task->set_extinfo(is_dt_needed ? nullptr : extinfo);
    task->set_dt_needed(is_dt_needed);

    LD_LOG(kLogDlopen, "find_libraries(ns=%s): task=%s, is_dt_needed=%d", ns->get_name(),
           task->get_name(), is_dt_needed);

    // Note: start from the namespace that is stored in the LoadTask. This namespace
    // is different from the current namespace when the LoadTask is for a transitive
    // dependency and the lib that created the LoadTask is not found in the
    // current namespace but in one of the linked namespace.
    if (!find_library_internal(const_cast<android_namespace_t*>(task->get_start_from()),
                               task,
                               &zip_archive_cache,
                               &load_tasks,
                               rtld_flags,
                               search_linked_namespaces || is_dt_needed)) {
      return false;
    }

    soinfo* si = task->get_soinfo();

    if (is_dt_needed) {
      needed_by->add_child(si);
    }

    // When ld_preloads is not null, the first
    // ld_preloads_count libs are in fact ld_preloads.
    if (ld_preloads != nullptr && soinfos_count < ld_preloads_count) {
      ld_preloads->push_back(si);
    }

    if (soinfos_count < library_names_count) {
      soinfos[soinfos_count++] = si;
    }
  }

  // Step 2: Load libraries in random order (see b/24047022)
  LoadTaskList load_list;
  for (auto&& task : load_tasks) {
    soinfo* si = task->get_soinfo();
    auto pred = [&](const LoadTask* t) {
      return t->get_soinfo() == si;
    };

    if (!si->is_linked() &&
        std::find_if(load_list.begin(), load_list.end(), pred) == load_list.end() ) {
      load_list.push_back(task);
    }
  }
  bool reserved_address_recursive = false;
  if (extinfo) {
    reserved_address_recursive = extinfo->flags & ANDROID_DLEXT_RESERVED_ADDRESS_RECURSIVE;
  }
  if (!reserved_address_recursive) {
    // Shuffle the load order in the normal case, but not if we are loading all
    // the libraries to a reserved address range.
    shuffle(&load_list);
  }

  // Set up address space parameters.
  address_space_params extinfo_params, default_params;
  size_t relro_fd_offset = 0;
  if (extinfo) {
    if (extinfo->flags & ANDROID_DLEXT_RESERVED_ADDRESS) {
      extinfo_params.start_addr = extinfo->reserved_addr;
      extinfo_params.reserved_size = extinfo->reserved_size;
      extinfo_params.must_use_address = true;
    } else if (extinfo->flags & ANDROID_DLEXT_RESERVED_ADDRESS_HINT) {
      extinfo_params.start_addr = extinfo->reserved_addr;
      extinfo_params.reserved_size = extinfo->reserved_size;
    }
  }

  for (auto&& task : load_list) {
    address_space_params* address_space =
        (reserved_address_recursive || !task->is_dt_needed()) ? &extinfo_params : &default_params;
    if (!task->load(address_space)) {
      return false;
    }
  }

  // Step 3: pre-link all DT_NEEDED libraries in breadth first order.
  for (auto&& task : load_tasks) {
    soinfo* si = task->get_soinfo();
    if (!si->is_linked() && !si->prelink_image()) {
      return false;
    }
    register_soinfo_tls(si);
  }

  // Step 4: Construct the global group. Note: DF_1_GLOBAL bit of a library is
  // determined at step 3.

  // Step 4-1: DF_1_GLOBAL bit is force set for LD_PRELOADed libs because they
  // must be added to the global group
  if (ld_preloads != nullptr) {
    for (auto&& si : *ld_preloads) {
      si->set_dt_flags_1(si->get_dt_flags_1() | DF_1_GLOBAL);
    }
  }

  // Step 4-2: Gather all DF_1_GLOBAL libs which were newly loaded during this
  // run. These will be the new member of the global group
  soinfo_list_t new_global_group_members;
  for (auto&& task : load_tasks) {
    soinfo* si = task->get_soinfo();
    if (!si->is_linked() && (si->get_dt_flags_1() & DF_1_GLOBAL) != 0) {
      new_global_group_members.push_back(si);
    }
  }

  // Step 4-3: Add the new global group members to all the linked namespaces
  if (namespaces != nullptr) {
    for (auto linked_ns : *namespaces) {
      for (auto si : new_global_group_members) {
        if (si->get_primary_namespace() != linked_ns) {
          linked_ns->add_soinfo(si);
          si->add_secondary_namespace(linked_ns);
        }
      }
    }
  }

  // Step 5: Collect roots of local_groups.
  // Whenever needed_by->si link crosses a namespace boundary it forms its own local_group.
  // Here we collect new roots to link them separately later on. Note that we need to avoid
  // collecting duplicates. Also the order is important. They need to be linked in the same
  // BFS order we link individual libraries.
  std::vector<soinfo*> local_group_roots;
  if (start_with != nullptr && add_as_children) {
    local_group_roots.push_back(start_with);
  } else {
    CHECK(soinfos_count == 1);
    local_group_roots.push_back(soinfos[0]);
  }

  for (auto&& task : load_tasks) {
    soinfo* si = task->get_soinfo();
    soinfo* needed_by = task->get_needed_by();
    bool is_dt_needed = needed_by != nullptr && (needed_by != start_with || add_as_children);
    android_namespace_t* needed_by_ns =
        is_dt_needed ? needed_by->get_primary_namespace() : ns;

    if (!si->is_linked() && si->get_primary_namespace() != needed_by_ns) {
      auto it = std::find(local_group_roots.begin(), local_group_roots.end(), si);
      LD_LOG(kLogDlopen,
             "Crossing namespace boundary (si=%s@%p, si_ns=%s@%p, needed_by=%s@%p, ns=%s@%p, needed_by_ns=%s@%p) adding to local_group_roots: %s",
             si->get_realpath(),
             si,
             si->get_primary_namespace()->get_name(),
             si->get_primary_namespace(),
             needed_by == nullptr ? "(nullptr)" : needed_by->get_realpath(),
             needed_by,
             ns->get_name(),
             ns,
             needed_by_ns->get_name(),
             needed_by_ns,
             it == local_group_roots.end() ? "yes" : "no");

      if (it == local_group_roots.end()) {
        local_group_roots.push_back(si);
      }
    }
  }

  // Step 6: Link all local groups
  for (auto root : local_group_roots) {
    soinfo_list_t local_group;
    android_namespace_t* local_group_ns = root->get_primary_namespace();

    walk_dependencies_tree(root,
      [&] (soinfo* si) {
        if (local_group_ns->is_accessible(si)) {
          local_group.push_back(si);
          return kWalkContinue;
        } else {
          return kWalkSkip;
        }
      });

    soinfo_list_t global_group = local_group_ns->get_global_group();
    bool linked = local_group.visit([&](soinfo* si) {
      // Even though local group may contain accessible soinfos from other namespaces
      // we should avoid linking them (because if they are not linked -> they
      // are in the local_group_roots and will be linked later).
      if (!si->is_linked() && si->get_primary_namespace() == local_group_ns) {
        const android_dlextinfo* link_extinfo = nullptr;
        if (si == soinfos[0] || reserved_address_recursive) {
          // Only forward extinfo for the first library unless the recursive
          // flag is set.
          link_extinfo = extinfo;
        }
        if (!si->link_image(global_group, local_group, link_extinfo, &relro_fd_offset) ||
            !get_cfi_shadow()->AfterLoad(si, solist_get_head())) {
          return false;
        }
      }

      return true;
    });

    if (!linked) {
      return false;
    }
  }

  // Step 7: Mark all load_tasks as linked and increment refcounts
  // for references between load_groups (at this point it does not matter if
  // referenced load_groups were loaded by previous dlopen or as part of this
  // one on step 6)
  if (start_with != nullptr && add_as_children) {
    start_with->set_linked();
  }

  for (auto&& task : load_tasks) {
    soinfo* si = task->get_soinfo();
    si->set_linked();
  }

  for (auto&& task : load_tasks) {
    soinfo* si = task->get_soinfo();
    soinfo* needed_by = task->get_needed_by();
    if (needed_by != nullptr &&
        needed_by != start_with &&
        needed_by->get_local_group_root() != si->get_local_group_root()) {
      si->increment_ref_count();
    }
  }


  return true;
}

static soinfo* find_library(android_namespace_t* ns,
                            const char* name, int rtld_flags,
                            const android_dlextinfo* extinfo,
                            soinfo* needed_by) {
  soinfo* si = nullptr;

  if (name == nullptr) {
    si = solist_get_somain();
  } else if (!find_libraries(ns,
                             needed_by,
                             &name,
                             1,
                             &si,
                             nullptr,
                             0,
                             rtld_flags,
                             extinfo,
                             false /* add_as_children */,
                             true /* search_linked_namespaces */)) {
    if (si != nullptr) {
      soinfo_unload(si);
    }
    return nullptr;
  }

  si->increment_ref_count();

  return si;
}

static void soinfo_unload_impl(soinfo* root) {
  ScopedTrace trace((std::string("unload ") + root->get_realpath()).c_str());
  bool is_linked = root->is_linked();

  if (!root->can_unload()) {
    LD_LOG(kLogDlopen,
           "... dlclose(root=\"%s\"@%p) ... not unloading - the load group is flagged with NODELETE",
           root->get_realpath(),
           root);
    return;
  }


  soinfo_list_t unload_list;
  unload_list.push_back(root);

  soinfo_list_t local_unload_list;
  soinfo_list_t external_unload_list;
  soinfo* si = nullptr;

  while ((si = unload_list.pop_front()) != nullptr) {
    if (local_unload_list.contains(si)) {
      continue;
    }

    local_unload_list.push_back(si);

    if (si->has_min_version(0)) {
      soinfo* child = nullptr;
      while ((child = si->get_children().pop_front()) != nullptr) {
        TRACE("%s@%p needs to unload %s@%p", si->get_realpath(), si,
            child->get_realpath(), child);

        child->get_parents().remove(si);

        if (local_unload_list.contains(child)) {
          continue;
        } else if (child->is_linked() && child->get_local_group_root() != root) {
          external_unload_list.push_back(child);
        } else if (child->get_parents().empty()) {
          unload_list.push_back(child);
        }
      }
    } else {
      async_safe_fatal("soinfo for \"%s\"@%p has no version", si->get_realpath(), si);
    }
  }

  local_unload_list.for_each([](soinfo* si) {
    LD_LOG(kLogDlopen,
           "... dlclose: calling destructors for \"%s\"@%p ... ",
           si->get_realpath(),
           si);
    si->call_destructors();
    LD_LOG(kLogDlopen,
           "... dlclose: calling destructors for \"%s\"@%p ... done",
           si->get_realpath(),
           si);
  });

  while ((si = local_unload_list.pop_front()) != nullptr) {
    LD_LOG(kLogDlopen,
           "... dlclose: unloading \"%s\"@%p ...",
           si->get_realpath(),
           si);
    notify_gdb_of_unload(si);
    unregister_soinfo_tls(si);
    get_cfi_shadow()->BeforeUnload(si);
    soinfo_free(si);
  }

  if (is_linked) {
    while ((si = external_unload_list.pop_front()) != nullptr) {
      LD_LOG(kLogDlopen,
             "... dlclose: unloading external reference \"%s\"@%p ...",
             si->get_realpath(),
             si);
      soinfo_unload(si);
    }
  } else {
      LD_LOG(kLogDlopen,
             "... dlclose: unload_si was not linked - not unloading external references ...");
  }
}

static void soinfo_unload(soinfo* unload_si) {
  // Note that the library can be loaded but not linked;
  // in which case there is no root but we still need
  // to walk the tree and unload soinfos involved.
  //
  // This happens on unsuccessful dlopen, when one of
  // the DT_NEEDED libraries could not be linked/found.
  bool is_linked = unload_si->is_linked();
  soinfo* root = is_linked ? unload_si->get_local_group_root() : unload_si;

  LD_LOG(kLogDlopen,
         "... dlclose(realpath=\"%s\"@%p) ... load group root is \"%s\"@%p",
         unload_si->get_realpath(),
         unload_si,
         root->get_realpath(),
         root);


  size_t ref_count = is_linked ? root->decrement_ref_count() : 0;
  if (ref_count > 0) {
    LD_LOG(kLogDlopen,
           "... dlclose(root=\"%s\"@%p) ... not unloading - decrementing ref_count to %zd",
           root->get_realpath(),
           root,
           ref_count);
    return;
  }

  soinfo_unload_impl(root);
}

void increment_dso_handle_reference_counter(void* dso_handle) {
  if (dso_handle == nullptr) {
    return;
  }

  auto it = g_dso_handle_counters.find(dso_handle);
  if (it != g_dso_handle_counters.end()) {
    CHECK(++it->second != 0);
  } else {
    soinfo* si = find_containing_library(dso_handle);
    if (si != nullptr) {
      ProtectedDataGuard guard;
      si->increment_ref_count();
    } else {
      async_safe_fatal(
          "increment_dso_handle_reference_counter: Couldn't find soinfo by dso_handle=%p",
          dso_handle);
    }
    g_dso_handle_counters[dso_handle] = 1U;
  }
}

void decrement_dso_handle_reference_counter(void* dso_handle) {
  if (dso_handle == nullptr) {
    return;
  }

  auto it = g_dso_handle_counters.find(dso_handle);
  CHECK(it != g_dso_handle_counters.end());
  CHECK(it->second != 0);

  if (--it->second == 0) {
    soinfo* si = find_containing_library(dso_handle);
    if (si != nullptr) {
      ProtectedDataGuard guard;
      soinfo_unload(si);
    } else {
      async_safe_fatal(
          "decrement_dso_handle_reference_counter: Couldn't find soinfo by dso_handle=%p",
          dso_handle);
    }
    g_dso_handle_counters.erase(it);
  }
}

static std::string symbol_display_name(const char* sym_name, const char* sym_ver) {
  if (sym_ver == nullptr) {
    return sym_name;
  }

  return std::string(sym_name) + ", version " + sym_ver;
}

static android_namespace_t* get_caller_namespace(soinfo* caller) {
  return caller != nullptr ? caller->get_primary_namespace() : g_anonymous_namespace;
}

void do_android_get_LD_LIBRARY_PATH(char* buffer, size_t buffer_size) {
  // Use basic string manipulation calls to avoid snprintf.
  // snprintf indirectly calls pthread_getspecific to get the size of a buffer.
  // When debug malloc is enabled, this call returns 0. This in turn causes
  // snprintf to do nothing, which causes libraries to fail to load.
  // See b/17302493 for further details.
  // Once the above bug is fixed, this code can be modified to use
  // snprintf again.
  const auto& default_ld_paths = g_default_namespace->get_default_library_paths();

  size_t required_size = 0;
  for (const auto& path : default_ld_paths) {
    required_size += path.size() + 1;
  }

  if (buffer_size < required_size) {
    async_safe_fatal("android_get_LD_LIBRARY_PATH failed, buffer too small: "
                     "buffer len %zu, required len %zu", buffer_size, required_size);
  }

  char* end = buffer;
  for (size_t i = 0; i < default_ld_paths.size(); ++i) {
    if (i > 0) *end++ = ':';
    end = stpcpy(end, default_ld_paths[i].c_str());
  }
}

void do_android_update_LD_LIBRARY_PATH(const char* ld_library_path) {
  parse_LD_LIBRARY_PATH(ld_library_path);
}

static std::string android_dlextinfo_to_string(const android_dlextinfo* info) {
  if (info == nullptr) {
    return "(null)";
  }

  return stringPrintf("[flags=0x%" PRIx64 ","
                                     " reserved_addr=%p,"
                                     " reserved_size=0x%zx,"
                                     " relro_fd=%d,"
                                     " library_fd=%d,"
                                     " library_fd_offset=0x%" PRIx64 ","
                                     " library_namespace=%s@%p]",
                                     info->flags,
                                     info->reserved_addr,
                                     info->reserved_size,
                                     info->relro_fd,
                                     info->library_fd,
                                     info->library_fd_offset,
                                     (info->flags & ANDROID_DLEXT_USE_NAMESPACE) != 0 ?
                                        (info->library_namespace != nullptr ?
                                          info->library_namespace->get_name() : "(null)") : "(n/a)",
                                     (info->flags & ANDROID_DLEXT_USE_NAMESPACE) != 0 ?
                                        info->library_namespace : nullptr);
}

void* do_dlopen(const char* name, int flags,
                const android_dlextinfo* extinfo,
                const void* caller_addr) {
  std::string trace_prefix = std::string("dlopen: ") + (name == nullptr ? "(nullptr)" : name);
  ScopedTrace trace(trace_prefix.c_str());
  ScopedTrace loading_trace((trace_prefix + " - loading and linking").c_str());
  soinfo* const caller = find_containing_library(caller_addr);
  android_namespace_t* ns = get_caller_namespace(caller);

  LD_LOG(kLogDlopen,
         "dlopen(name=\"%s\", flags=0x%x, extinfo=%s, caller=\"%s\", caller_ns=%s@%p, targetSdkVersion=%i) ...",
         name,
         flags,
         android_dlextinfo_to_string(extinfo).c_str(),
         caller == nullptr ? "(null)" : caller->get_realpath(),
         ns == nullptr ? "(null)" : ns->get_name(),
         ns,
         get_application_target_sdk_version());

  auto purge_guard = android::base::make_scope_guard([&]() { purge_unused_memory(); });

  auto failure_guard = android::base::make_scope_guard(
      [&]() { LD_LOG(kLogDlopen, "... dlopen failed: %s", linker_get_error_buffer()); });

  if ((flags & ~(RTLD_NOW|RTLD_LAZY|RTLD_LOCAL|RTLD_GLOBAL|RTLD_NODELETE|RTLD_NOLOAD)) != 0) {
    DL_ERR("invalid flags to dlopen: %x", flags);
    return nullptr;
  }

  if (extinfo != nullptr) {
    if ((extinfo->flags & ~(ANDROID_DLEXT_VALID_FLAG_BITS)) != 0) {
      DL_ERR("invalid extended flags to android_dlopen_ext: 0x%" PRIx64, extinfo->flags);
      return nullptr;
    }

    if ((extinfo->flags & ANDROID_DLEXT_USE_LIBRARY_FD) == 0 &&
        (extinfo->flags & ANDROID_DLEXT_USE_LIBRARY_FD_OFFSET) != 0) {
      DL_ERR("invalid extended flag combination (ANDROID_DLEXT_USE_LIBRARY_FD_OFFSET without "
          "ANDROID_DLEXT_USE_LIBRARY_FD): 0x%" PRIx64, extinfo->flags);
      return nullptr;
    }

    if ((extinfo->flags & ANDROID_DLEXT_USE_NAMESPACE) != 0) {
      if (extinfo->library_namespace == nullptr) {
        DL_ERR("ANDROID_DLEXT_USE_NAMESPACE is set but extinfo->library_namespace is null");
        return nullptr;
      }
      ns = extinfo->library_namespace;
    }
  }

  // Workaround for dlopen(/system/lib/<soname>) when .so is in /apex. http://b/121248172
  // The workaround works only when targetSdkVersion < Q.
  std::string name_to_apex;
  if (translateSystemPathToApexPath(name, &name_to_apex)) {
    const char* new_name = name_to_apex.c_str();
    LD_LOG(kLogDlopen, "dlopen considering translation from %s to APEX path %s",
           name,
           new_name);
    // Some APEXs could be optionally disabled. Only translate the path
    // when the old file is absent and the new file exists.
    // TODO(b/124218500): Re-enable it once app compat issue is resolved
    /*
    if (file_exists(name)) {
      LD_LOG(kLogDlopen, "dlopen %s exists, not translating", name);
    } else
    */
    if (!file_exists(new_name)) {
      LD_LOG(kLogDlopen, "dlopen %s does not exist, not translating",
             new_name);
    } else {
      LD_LOG(kLogDlopen, "dlopen translation accepted: using %s", new_name);
      name = new_name;
    }
  }
  // End Workaround for dlopen(/system/lib/<soname>) when .so is in /apex.

  std::string asan_name_holder;

  const char* translated_name = name;
  if (g_is_asan && translated_name != nullptr && translated_name[0] == '/') {
    char original_path[PATH_MAX];
    if (realpath(name, original_path) != nullptr) {
      asan_name_holder = std::string(kAsanLibDirPrefix) + original_path;
      if (file_exists(asan_name_holder.c_str())) {
        soinfo* si = nullptr;
        if (find_loaded_library_by_realpath(ns, original_path, true, &si)) {
          PRINT("linker_asan dlopen NOT translating \"%s\" -> \"%s\": library already loaded", name,
                asan_name_holder.c_str());
        } else {
          PRINT("linker_asan dlopen translating \"%s\" -> \"%s\"", name, translated_name);
          translated_name = asan_name_holder.c_str();
        }
      }
    }
  }

  ProtectedDataGuard guard;
  soinfo* si = find_library(ns, translated_name, flags, extinfo, caller);
  loading_trace.End();

  if (si != nullptr) {
    void* handle = si->to_handle();
    LD_LOG(kLogDlopen,
           "... dlopen calling constructors: realpath=\"%s\", soname=\"%s\", handle=%p",
           si->get_realpath(), si->get_soname(), handle);
    si->call_constructors();
    failure_guard.Disable();
    LD_LOG(kLogDlopen,
           "... dlopen successful: realpath=\"%s\", soname=\"%s\", handle=%p",
           si->get_realpath(), si->get_soname(), handle);
    return handle;
  }

  return nullptr;
}

int do_dladdr(const void* addr, Dl_info* info) {
  // Determine if this address can be found in any library currently mapped.
  soinfo* si = find_containing_library(addr);
  if (si == nullptr) {
    return 0;
  }

  memset(info, 0, sizeof(Dl_info));

  info->dli_fname = si->get_realpath();
  // Address at which the shared object is loaded.
  info->dli_fbase = reinterpret_cast<void*>(si->base);

  // Determine if any symbol in the library contains the specified address.
  ElfW(Sym)* sym = si->find_symbol_by_address(addr);
  if (sym != nullptr) {
    info->dli_sname = si->get_string(sym->st_name);
    info->dli_saddr = reinterpret_cast<void*>(si->resolve_symbol_address(sym));
  }

  return 1;
}

static soinfo* soinfo_from_handle(void* handle) {
  if ((reinterpret_cast<uintptr_t>(handle) & 1) != 0) {
    auto it = g_soinfo_handles_map.find(reinterpret_cast<uintptr_t>(handle));
    if (it == g_soinfo_handles_map.end()) {
      return nullptr;
    } else {
      return it->second;
    }
  }

  return static_cast<soinfo*>(handle);
}

bool do_dlsym(void* handle,
              const char* sym_name,
              const char* sym_ver,
              const void* caller_addr,
              void** symbol) {
  ScopedTrace trace("dlsym");
#if !defined(__LP64__)
  if (handle == nullptr) {
    DL_ERR("dlsym failed: library handle is null");
    return false;
  }
#endif

  soinfo* found = nullptr;
  const ElfW(Sym)* sym = nullptr;
  soinfo* caller = find_containing_library(caller_addr);
  android_namespace_t* ns = get_caller_namespace(caller);
  soinfo* si = nullptr;
  if (handle != RTLD_DEFAULT && handle != RTLD_NEXT) {
    si = soinfo_from_handle(handle);
  }

  LD_LOG(kLogDlsym,
         "dlsym(handle=%p(\"%s\"), sym_name=\"%s\", sym_ver=\"%s\", caller=\"%s\", caller_ns=%s@%p) ...",
         handle,
         si != nullptr ? si->get_realpath() : "n/a",
         sym_name,
         sym_ver,
         caller == nullptr ? "(null)" : caller->get_realpath(),
         ns == nullptr ? "(null)" : ns->get_name(),
         ns);

  auto failure_guard = android::base::make_scope_guard(
      [&]() { LD_LOG(kLogDlsym, "... dlsym failed: %s", linker_get_error_buffer()); });

  if (sym_name == nullptr) {
    DL_ERR("dlsym failed: symbol name is null");
    return false;
  }

  version_info vi_instance;
  version_info* vi = nullptr;

  if (sym_ver != nullptr) {
    vi_instance.name = sym_ver;
    vi_instance.elf_hash = calculate_elf_hash(sym_ver);
    vi = &vi_instance;
  }

  if (handle == RTLD_DEFAULT || handle == RTLD_NEXT) {
    sym = dlsym_linear_lookup(ns, sym_name, vi, &found, caller, handle);
  } else {
    if (si == nullptr) {
      DL_ERR("dlsym failed: invalid handle: %p", handle);
      return false;
    }
    sym = dlsym_handle_lookup(si, &found, sym_name, vi);
  }

  if (sym != nullptr) {
    uint32_t bind = ELF_ST_BIND(sym->st_info);
    uint32_t type = ELF_ST_TYPE(sym->st_info);

    if ((bind == STB_GLOBAL || bind == STB_WEAK) && sym->st_shndx != 0) {
      if (type == STT_TLS) {
        // For a TLS symbol, dlsym returns the address of the current thread's
        // copy of the symbol. This function may allocate a DTV and/or storage
        // for the source TLS module. (Allocating a DTV isn't necessary if the
        // symbol is part of static TLS, but it's simpler to reuse
        // __tls_get_addr.)
        soinfo_tls* tls_module = found->get_tls();
        if (tls_module == nullptr) {
          DL_ERR("TLS symbol \"%s\" in solib \"%s\" with no TLS segment",
                 sym_name, found->get_realpath());
          return false;
        }
        const TlsIndex ti { tls_module->module_id, sym->st_value };
        *symbol = TLS_GET_ADDR(&ti);
      } else {
        *symbol = reinterpret_cast<void*>(found->resolve_symbol_address(sym));
      }
      failure_guard.Disable();
      LD_LOG(kLogDlsym,
             "... dlsym successful: sym_name=\"%s\", sym_ver=\"%s\", found in=\"%s\", address=%p",
             sym_name, sym_ver, found->get_soname(), *symbol);
      return true;
    }

    DL_ERR("symbol \"%s\" found but not global", symbol_display_name(sym_name, sym_ver).c_str());
    return false;
  }

  DL_ERR("undefined symbol: %s", symbol_display_name(sym_name, sym_ver).c_str());
  return false;
}

int do_dlclose(void* handle) {
  ScopedTrace trace("dlclose");
  ProtectedDataGuard guard;
  soinfo* si = soinfo_from_handle(handle);
  if (si == nullptr) {
    DL_ERR("invalid handle: %p", handle);
    return -1;
  }

  LD_LOG(kLogDlopen,
         "dlclose(handle=%p, realpath=\"%s\"@%p) ...",
         handle,
         si->get_realpath(),
         si);
  soinfo_unload(si);
  LD_LOG(kLogDlopen,
         "dlclose(handle=%p) ... done",
         handle);
  return 0;
}

bool init_anonymous_namespace(const char* shared_lib_sonames, const char* library_search_path) {
  if (g_anonymous_namespace_initialized) {
    DL_ERR("anonymous namespace has already been initialized.");
    return false;
  }

  ProtectedDataGuard guard;

  // create anonymous namespace
  // When the caller is nullptr - create_namespace will take global group
  // from the anonymous namespace, which is fine because anonymous namespace
  // is still pointing to the default one.
  android_namespace_t* anon_ns =
      create_namespace(nullptr,
                       "(anonymous)",
                       nullptr,
                       library_search_path,
                       ANDROID_NAMESPACE_TYPE_ISOLATED,
                       nullptr,
                       g_default_namespace);

  if (anon_ns == nullptr) {
    return false;
  }

  if (!link_namespaces(anon_ns, g_default_namespace, shared_lib_sonames)) {
    return false;
  }

  g_anonymous_namespace = anon_ns;
  g_anonymous_namespace_initialized = true;

  return true;
}

static void add_soinfos_to_namespace(const soinfo_list_t& soinfos, android_namespace_t* ns) {
  ns->add_soinfos(soinfos);
  for (auto si : soinfos) {
    si->add_secondary_namespace(ns);
  }
}

android_namespace_t* create_namespace(const void* caller_addr,
                                      const char* name,
                                      const char* ld_library_path,
                                      const char* default_library_path,
                                      uint64_t type,
                                      const char* permitted_when_isolated_path,
                                      android_namespace_t* parent_namespace) {
  if (parent_namespace == nullptr) {
    // if parent_namespace is nullptr -> set it to the caller namespace
    soinfo* caller_soinfo = find_containing_library(caller_addr);

    parent_namespace = caller_soinfo != nullptr ?
                       caller_soinfo->get_primary_namespace() :
                       g_anonymous_namespace;
  }

  ProtectedDataGuard guard;
  std::vector<std::string> ld_library_paths;
  std::vector<std::string> default_library_paths;
  std::vector<std::string> permitted_paths;

  parse_path(ld_library_path, ":", &ld_library_paths);
  parse_path(default_library_path, ":", &default_library_paths);
  parse_path(permitted_when_isolated_path, ":", &permitted_paths);

  android_namespace_t* ns = new (g_namespace_allocator.alloc()) android_namespace_t();
  ns->set_name(name);
  ns->set_isolated((type & ANDROID_NAMESPACE_TYPE_ISOLATED) != 0);
  ns->set_greylist_enabled((type & ANDROID_NAMESPACE_TYPE_GREYLIST_ENABLED) != 0);

  if ((type & ANDROID_NAMESPACE_TYPE_SHARED) != 0) {
    // append parent namespace paths.
    std::copy(parent_namespace->get_ld_library_paths().begin(),
              parent_namespace->get_ld_library_paths().end(),
              back_inserter(ld_library_paths));

    std::copy(parent_namespace->get_default_library_paths().begin(),
              parent_namespace->get_default_library_paths().end(),
              back_inserter(default_library_paths));

    std::copy(parent_namespace->get_permitted_paths().begin(),
              parent_namespace->get_permitted_paths().end(),
              back_inserter(permitted_paths));

    // If shared - clone the parent namespace
    add_soinfos_to_namespace(parent_namespace->soinfo_list(), ns);
    // and copy parent namespace links
    for (auto& link : parent_namespace->linked_namespaces()) {
      ns->add_linked_namespace(link.linked_namespace(), link.shared_lib_sonames(),
                               link.allow_all_shared_libs());
    }
  } else {
    // If not shared - copy only the shared group
    add_soinfos_to_namespace(parent_namespace->get_shared_group(), ns);
  }

  ns->set_ld_library_paths(std::move(ld_library_paths));
  ns->set_default_library_paths(std::move(default_library_paths));
  ns->set_permitted_paths(std::move(permitted_paths));

  return ns;
}

bool link_namespaces(android_namespace_t* namespace_from,
                     android_namespace_t* namespace_to,
                     const char* shared_lib_sonames) {
  if (namespace_to == nullptr) {
    namespace_to = g_default_namespace;
  }

  if (namespace_from == nullptr) {
    DL_ERR("error linking namespaces: namespace_from is null.");
    return false;
  }

  if (shared_lib_sonames == nullptr || shared_lib_sonames[0] == '\0') {
    DL_ERR("error linking namespaces \"%s\"->\"%s\": the list of shared libraries is empty.",
           namespace_from->get_name(), namespace_to->get_name());
    return false;
  }

  auto sonames = split(shared_lib_sonames, ":");
  std::unordered_set<std::string> sonames_set(sonames.begin(), sonames.end());

  ProtectedDataGuard guard;
  namespace_from->add_linked_namespace(namespace_to, sonames_set, false);

  return true;
}

bool link_namespaces_all_libs(android_namespace_t* namespace_from,
                              android_namespace_t* namespace_to) {
  if (namespace_from == nullptr) {
    DL_ERR("error linking namespaces: namespace_from is null.");
    return false;
  }

  if (namespace_to == nullptr) {
    DL_ERR("error linking namespaces: namespace_to is null.");
    return false;
  }

  ProtectedDataGuard guard;
  namespace_from->add_linked_namespace(namespace_to, std::unordered_set<std::string>(), true);

  return true;
}

ElfW(Addr) call_ifunc_resolver(ElfW(Addr) resolver_addr) {
  typedef ElfW(Addr) (*ifunc_resolver_t)(void);
  ifunc_resolver_t ifunc_resolver = reinterpret_cast<ifunc_resolver_t>(resolver_addr);
  ElfW(Addr) ifunc_addr = ifunc_resolver();
  TRACE_TYPE(RELO, "Called ifunc_resolver@%p. The result is %p",
      ifunc_resolver, reinterpret_cast<void*>(ifunc_addr));

  return ifunc_addr;
}

const version_info* VersionTracker::get_version_info(ElfW(Versym) source_symver) const {
  if (source_symver < 2 ||
      source_symver >= version_infos.size() ||
      version_infos[source_symver].name == nullptr) {
    return nullptr;
  }

  return &version_infos[source_symver];
}

void VersionTracker::add_version_info(size_t source_index,
                                      ElfW(Word) elf_hash,
                                      const char* ver_name,
                                      const soinfo* target_si) {
  if (source_index >= version_infos.size()) {
    version_infos.resize(source_index+1);
  }

  version_infos[source_index].elf_hash = elf_hash;
  version_infos[source_index].name = ver_name;
  version_infos[source_index].target_si = target_si;
}

bool VersionTracker::init_verneed(const soinfo* si_from) {
  uintptr_t verneed_ptr = si_from->get_verneed_ptr();

  if (verneed_ptr == 0) {
    return true;
  }

  size_t verneed_cnt = si_from->get_verneed_cnt();

  for (size_t i = 0, offset = 0; i<verneed_cnt; ++i) {
    const ElfW(Verneed)* verneed = reinterpret_cast<ElfW(Verneed)*>(verneed_ptr + offset);
    size_t vernaux_offset = offset + verneed->vn_aux;
    offset += verneed->vn_next;

    if (verneed->vn_version != 1) {
      DL_ERR("unsupported verneed[%zd] vn_version: %d (expected 1)", i, verneed->vn_version);
      return false;
    }

    const char* target_soname = si_from->get_string(verneed->vn_file);
    // find it in dependencies
    soinfo* target_si = si_from->get_children().find_if([&](const soinfo* si) {
      return si->get_soname() != nullptr && strcmp(si->get_soname(), target_soname) == 0;
    });

    if (target_si == nullptr) {
      DL_ERR("cannot find \"%s\" from verneed[%zd] in DT_NEEDED list for \"%s\"",
          target_soname, i, si_from->get_realpath());
      return false;
    }

    for (size_t j = 0; j<verneed->vn_cnt; ++j) {
      const ElfW(Vernaux)* vernaux = reinterpret_cast<ElfW(Vernaux)*>(verneed_ptr + vernaux_offset);
      vernaux_offset += vernaux->vna_next;

      const ElfW(Word) elf_hash = vernaux->vna_hash;
      const char* ver_name = si_from->get_string(vernaux->vna_name);
      ElfW(Half) source_index = vernaux->vna_other;

      add_version_info(source_index, elf_hash, ver_name, target_si);
    }
  }

  return true;
}

template <typename F>
static bool for_each_verdef(const soinfo* si, F functor) {
  if (!si->has_min_version(2)) {
    return true;
  }

  uintptr_t verdef_ptr = si->get_verdef_ptr();
  if (verdef_ptr == 0) {
    return true;
  }

  size_t offset = 0;

  size_t verdef_cnt = si->get_verdef_cnt();
  for (size_t i = 0; i<verdef_cnt; ++i) {
    const ElfW(Verdef)* verdef = reinterpret_cast<ElfW(Verdef)*>(verdef_ptr + offset);
    size_t verdaux_offset = offset + verdef->vd_aux;
    offset += verdef->vd_next;

    if (verdef->vd_version != 1) {
      DL_ERR("unsupported verdef[%zd] vd_version: %d (expected 1) library: %s",
          i, verdef->vd_version, si->get_realpath());
      return false;
    }

    if ((verdef->vd_flags & VER_FLG_BASE) != 0) {
      // "this is the version of the file itself.  It must not be used for
      //  matching a symbol. It can be used to match references."
      //
      // http://www.akkadia.org/drepper/symbol-versioning
      continue;
    }

    if (verdef->vd_cnt == 0) {
      DL_ERR("invalid verdef[%zd] vd_cnt == 0 (version without a name)", i);
      return false;
    }

    const ElfW(Verdaux)* verdaux = reinterpret_cast<ElfW(Verdaux)*>(verdef_ptr + verdaux_offset);

    if (functor(i, verdef, verdaux) == true) {
      break;
    }
  }

  return true;
}

bool find_verdef_version_index(const soinfo* si, const version_info* vi, ElfW(Versym)* versym) {
  if (vi == nullptr) {
    *versym = kVersymNotNeeded;
    return true;
  }

  *versym = kVersymGlobal;

  return for_each_verdef(si,
    [&](size_t, const ElfW(Verdef)* verdef, const ElfW(Verdaux)* verdaux) {
      if (verdef->vd_hash == vi->elf_hash &&
          strcmp(vi->name, si->get_string(verdaux->vda_name)) == 0) {
        *versym = verdef->vd_ndx;
        return true;
      }

      return false;
    }
  );
}

bool VersionTracker::init_verdef(const soinfo* si_from) {
  return for_each_verdef(si_from,
    [&](size_t, const ElfW(Verdef)* verdef, const ElfW(Verdaux)* verdaux) {
      add_version_info(verdef->vd_ndx, verdef->vd_hash,
          si_from->get_string(verdaux->vda_name), si_from);
      return false;
    }
  );
}

bool VersionTracker::init(const soinfo* si_from) {
  if (!si_from->has_min_version(2)) {
    return true;
  }

  return init_verneed(si_from) && init_verdef(si_from);
}

// TODO (dimitry): Methods below need to be moved out of soinfo
// and in more isolated file in order minimize dependencies on
// unnecessary object in the linker binary. Consider making them
// independent from soinfo (?).
bool soinfo::lookup_version_info(const VersionTracker& version_tracker, ElfW(Word) sym,
                                 const char* sym_name, const version_info** vi) {
  const ElfW(Versym)* sym_ver_ptr = get_versym(sym);
  ElfW(Versym) sym_ver = sym_ver_ptr == nullptr ? 0 : *sym_ver_ptr;

  if (sym_ver != VER_NDX_LOCAL && sym_ver != VER_NDX_GLOBAL) {
    *vi = version_tracker.get_version_info(sym_ver);

    if (*vi == nullptr) {
      DL_ERR("cannot find verneed/verdef for version index=%d "
          "referenced by symbol \"%s\" at \"%s\"", sym_ver, sym_name, get_realpath());
      return false;
    }
  } else {
    // there is no version info
    *vi = nullptr;
  }

  return true;
}

void soinfo::apply_relr_reloc(ElfW(Addr) offset) {
  ElfW(Addr) address = offset + load_bias;
  *reinterpret_cast<ElfW(Addr)*>(address) += load_bias;
}

// Process relocations in SHT_RELR section (experimental).
// Details of the encoding are described in this post:
//   https://groups.google.com/d/msg/generic-abi/bX460iggiKg/Pi9aSwwABgAJ
bool soinfo::relocate_relr() {
  ElfW(Relr)* begin = relr_;
  ElfW(Relr)* end = relr_ + relr_count_;
  constexpr size_t wordsize = sizeof(ElfW(Addr));

  ElfW(Addr) base = 0;
  for (ElfW(Relr)* current = begin; current < end; ++current) {
    ElfW(Relr) entry = *current;
    ElfW(Addr) offset;

    if ((entry&1) == 0) {
      // Even entry: encodes the offset for next relocation.
      offset = static_cast<ElfW(Addr)>(entry);
      apply_relr_reloc(offset);
      // Set base offset for subsequent bitmap entries.
      base = offset + wordsize;
      continue;
    }

    // Odd entry: encodes bitmap for relocations starting at base.
    offset = base;
    while (entry != 0) {
      entry >>= 1;
      if ((entry&1) != 0) {
        apply_relr_reloc(offset);
      }
      offset += wordsize;
    }

    // Advance base offset by 63 words for 64-bit platforms,
    // or 31 words for 32-bit platforms.
    base += (8*wordsize - 1) * wordsize;
  }
  return true;
}

#if !defined(__mips__)
#if defined(USE_RELA)
static ElfW(Addr) get_addend(ElfW(Rela)* rela, ElfW(Addr) reloc_addr __unused) {
  return rela->r_addend;
}
#else
static ElfW(Addr) get_addend(ElfW(Rel)* rel, ElfW(Addr) reloc_addr) {
  if (ELFW(R_TYPE)(rel->r_info) == R_GENERIC_RELATIVE ||
      ELFW(R_TYPE)(rel->r_info) == R_GENERIC_IRELATIVE ||
      ELFW(R_TYPE)(rel->r_info) == R_GENERIC_TLS_DTPREL ||
      ELFW(R_TYPE)(rel->r_info) == R_GENERIC_TLS_TPREL) {
    return *reinterpret_cast<ElfW(Addr)*>(reloc_addr);
  }
  return 0;
}
#endif

static bool is_tls_reloc(ElfW(Word) type) {
  switch (type) {
    case R_GENERIC_TLS_DTPMOD:
    case R_GENERIC_TLS_DTPREL:
    case R_GENERIC_TLS_TPREL:
    case R_GENERIC_TLSDESC:
      return true;
    default:
      return false;
  }
}

template<typename ElfRelIteratorT>
bool soinfo::relocate(const VersionTracker& version_tracker, ElfRelIteratorT&& rel_iterator,
                      const soinfo_list_t& global_group, const soinfo_list_t& local_group) {
  const size_t tls_tp_base = 0/*__libc_shared_globals()->static_tls_layout.offset_thread_pointer()*/;
  std::vector<std::pair<TlsDescriptor*, size_t>> deferred_tlsdesc_relocs;

  for (size_t idx = 0; rel_iterator.has_next(); ++idx) {
    const auto rel = rel_iterator.next();
    if (rel == nullptr) {
      return false;
    }

    ElfW(Word) type = ELFW(R_TYPE)(rel->r_info);
    ElfW(Word) sym = ELFW(R_SYM)(rel->r_info);

    ElfW(Addr) reloc = static_cast<ElfW(Addr)>(rel->r_offset + load_bias);
    ElfW(Addr) sym_addr = 0;
    const char* sym_name = nullptr;
    ElfW(Addr) addend = get_addend(rel, reloc);

    DEBUG("Processing \"%s\" relocation at index %zd", get_realpath(), idx);
    if (type == R_GENERIC_NONE) {
      continue;
    }

    const ElfW(Sym)* s = nullptr;
    soinfo* lsi = nullptr;

    if (sym == 0) {
      // By convention in ld.bfd and lld, an omitted symbol on a TLS relocation
      // is a reference to the current module.
      if (is_tls_reloc(type)) {
        lsi = this;
      }
    } else if (ELF_ST_BIND(symtab_[sym].st_info) == STB_LOCAL && is_tls_reloc(type)) {
      // In certain situations, the Gold linker accesses a TLS symbol using a
      // relocation to an STB_LOCAL symbol in .dynsym of either STT_SECTION or
      // STT_TLS type. Bionic doesn't support these relocations, so issue an
      // error. References:
      //  - https://groups.google.com/d/topic/generic-abi/dJ4_Y78aQ2M/discussion
      //  - https://sourceware.org/bugzilla/show_bug.cgi?id=17699
      s = &symtab_[sym];
      sym_name = get_string(s->st_name);
      DL_ERR("unexpected TLS reference to local symbol \"%s\": "
             "sym type %d, rel type %u (idx %zu of \"%s\")",
             sym_name, ELF_ST_TYPE(s->st_info), type, idx, get_realpath());
      return false;
    } else {
      sym_name = get_string(symtab_[sym].st_name);
      const version_info* vi = nullptr;

      sym_addr = reinterpret_cast<ElfW(Addr)>(_get_hooked_symbol(sym_name, get_realpath()));

      if(!sym_addr) {

      if (!lookup_version_info(version_tracker, sym, sym_name, &vi)) {
        return false;
      }

      if (!soinfo_do_lookup(this, sym_name, vi, &lsi, global_group, local_group, &s)) {
        return false;
      }
      }

      if (sym_addr == 0 && s == nullptr) {
        // We only allow an undefined symbol if this is a weak reference...
        s = &symtab_[sym];
        if (ELF_ST_BIND(s->st_info) != STB_WEAK) {
          DL_ERR("cannot locate symbol \"%s\" referenced by \"%s\"...", sym_name, get_realpath());
          return false;
        }

        /* IHI0044C AAELF 4.5.1.1:

           Libraries are not searched to resolve weak references.
           It is not an error for a weak reference to remain unsatisfied.

           During linking, the value of an undefined weak reference is:
           - Zero if the relocation type is absolute
           - The address of the place if the relocation is pc-relative
           - The address of nominal base address if the relocation
             type is base-relative.
         */

        switch (type) {
          case R_GENERIC_JUMP_SLOT:
          case R_GENERIC_GLOB_DAT:
          case R_GENERIC_RELATIVE:
          case R_GENERIC_IRELATIVE:
          case R_GENERIC_TLS_DTPMOD:
          case R_GENERIC_TLS_DTPREL:
          case R_GENERIC_TLS_TPREL:
          case R_GENERIC_TLSDESC:
#if defined(__aarch64__)
          case R_AARCH64_ABS64:
          case R_AARCH64_ABS32:
          case R_AARCH64_ABS16:
#elif defined(__x86_64__)
          case R_X86_64_32:
          case R_X86_64_64:
#elif defined(__arm__)
          case R_ARM_ABS32:
#elif defined(__i386__)
          case R_386_32:
#endif
            /*
             * The sym_addr was initialized to be zero above, or the relocation
             * code below does not care about value of sym_addr.
             * No need to do anything.
             */
            break;
#if defined(__x86_64__)
          case R_X86_64_PC32:
            sym_addr = reloc;
            break;
#elif defined(__i386__)
          case R_386_PC32:
            sym_addr = reloc;
            break;
#endif
          default:
            DL_ERR("unknown weak reloc type %d @ %p (%zu)", type, rel, idx);
            return false;
        }
      } else if (sym_addr == 0) { // We got a definition.
#if !defined(__LP64__)
        // When relocating dso with text_relocation .text segment is
        // not executable. We need to restore elf flags before resolving
        // STT_GNU_IFUNC symbol.
        bool protect_segments = has_text_relocations &&
                                lsi == this &&
                                ELF_ST_TYPE(s->st_info) == STT_GNU_IFUNC;
        if (protect_segments) {
          if (phdr_table_protect_segments(phdr, phnum, load_bias) < 0) {
            DL_ERR("can't protect segments for \"%s\": %s",
                   get_realpath(), strerror(errno));
            return false;
          }
        }
#endif
        if (is_tls_reloc(type)) {
          if (ELF_ST_TYPE(s->st_info) != STT_TLS) {
            DL_ERR("reference to non-TLS symbol \"%s\" from TLS relocation in \"%s\"",
                   sym_name, get_realpath());
            return false;
          }
          if (lsi->get_tls() == nullptr) {
            DL_ERR("TLS relocation refers to symbol \"%s\" in solib \"%s\" with no TLS segment",
                   sym_name, lsi->get_realpath());
            return false;
          }
          sym_addr = s->st_value;
        } else {
          if (ELF_ST_TYPE(s->st_info) == STT_TLS) {
            DL_ERR("reference to TLS symbol \"%s\" from non-TLS relocation in \"%s\"",
                   sym_name, get_realpath());
            return false;
          }
          sym_addr = lsi->resolve_symbol_address(s);
        }
#if !defined(__LP64__)
        if (protect_segments) {
          if (phdr_table_unprotect_segments(phdr, phnum, load_bias) < 0) {
            DL_ERR("can't unprotect loadable segments for \"%s\": %s",
                   get_realpath(), strerror(errno));
            return false;
          }
        }
#endif
      }
      count_relocation(kRelocSymbol);
    }

    switch (type) {
      case R_GENERIC_JUMP_SLOT:
        count_relocation(kRelocAbsolute);
        MARK(rel->r_offset);
        TRACE_TYPE(RELO, "RELO JMP_SLOT %16p <- %16p %s\n",
                   reinterpret_cast<void*>(reloc),
                   reinterpret_cast<void*>(sym_addr + addend), sym_name);

        *reinterpret_cast<ElfW(Addr)*>(reloc) = (sym_addr + addend);
        break;
      case R_GENERIC_GLOB_DAT:
        count_relocation(kRelocAbsolute);
        MARK(rel->r_offset);
        TRACE_TYPE(RELO, "RELO GLOB_DAT %16p <- %16p %s\n",
                   reinterpret_cast<void*>(reloc),
                   reinterpret_cast<void*>(sym_addr + addend), sym_name);
        *reinterpret_cast<ElfW(Addr)*>(reloc) = (sym_addr + addend);
        break;
      case R_GENERIC_RELATIVE:
        count_relocation(kRelocRelative);
        MARK(rel->r_offset);
        TRACE_TYPE(RELO, "RELO RELATIVE %16p <- %16p\n",
                   reinterpret_cast<void*>(reloc),
                   reinterpret_cast<void*>(load_bias + addend));
        *reinterpret_cast<ElfW(Addr)*>(reloc) = (load_bias + addend);
        break;
      case R_GENERIC_IRELATIVE:
        count_relocation(kRelocRelative);
        MARK(rel->r_offset);
        TRACE_TYPE(RELO, "RELO IRELATIVE %16p <- %16p\n",
                    reinterpret_cast<void*>(reloc),
                    reinterpret_cast<void*>(load_bias + addend));
        {
#if !defined(__LP64__)
          // When relocating dso with text_relocation .text segment is
          // not executable. We need to restore elf flags for this
          // particular call.
          if (has_text_relocations) {
            if (phdr_table_protect_segments(phdr, phnum, load_bias) < 0) {
              DL_ERR("can't protect segments for \"%s\": %s",
                     get_realpath(), strerror(errno));
              return false;
            }
          }
#endif
          ElfW(Addr) ifunc_addr = call_ifunc_resolver(load_bias + addend);
#if !defined(__LP64__)
          // Unprotect it afterwards...
          if (has_text_relocations) {
            if (phdr_table_unprotect_segments(phdr, phnum, load_bias) < 0) {
              DL_ERR("can't unprotect loadable segments for \"%s\": %s",
                     get_realpath(), strerror(errno));
              return false;
            }
          }
#endif
          *reinterpret_cast<ElfW(Addr)*>(reloc) = ifunc_addr;
        }
        break;
      case R_GENERIC_TLS_TPREL:
        count_relocation(kRelocRelative);
        MARK(rel->r_offset);
        {
          ElfW(Addr) tpoff = 0;
          if (lsi == nullptr) {
            // Unresolved weak relocation. Leave tpoff at 0 to resolve
            // &weak_tls_symbol to __get_tls().
          } else {
            CHECK(lsi->get_tls() != nullptr); // We rejected a missing TLS segment above.
            const TlsModule& mod = get_tls_module(lsi->get_tls()->module_id);
            if (mod.static_offset != SIZE_MAX) {
              tpoff += mod.static_offset - tls_tp_base;
            } else {
              DL_ERR("TLS symbol \"%s\" in dlopened \"%s\" referenced from \"%s\" using IE access model",
                     sym_name, lsi->get_realpath(), get_realpath());
              return false;
            }
          }
          tpoff += sym_addr + addend;
          TRACE_TYPE(RELO, "RELO TLS_TPREL %16p <- %16p %s\n",
                     reinterpret_cast<void*>(reloc),
                     reinterpret_cast<void*>(tpoff), sym_name);
          *reinterpret_cast<ElfW(Addr)*>(reloc) = tpoff;
        }
        break;

#if !defined(__aarch64__)
      // Omit support for DTPMOD/DTPREL on arm64, at least until
      // http://b/123385182 is fixed. arm64 uses TLSDESC instead.
      case R_GENERIC_TLS_DTPMOD:
        count_relocation(kRelocRelative);
        MARK(rel->r_offset);
        {
          size_t module_id = 0;
          if (lsi == nullptr) {
            // Unresolved weak relocation. Evaluate the module ID to 0.
          } else {
            CHECK(lsi->get_tls() != nullptr); // We rejected a missing TLS segment above.
            module_id = lsi->get_tls()->module_id;
          }
          TRACE_TYPE(RELO, "RELO TLS_DTPMOD %16p <- %zu %s\n",
                     reinterpret_cast<void*>(reloc), module_id, sym_name);
          *reinterpret_cast<ElfW(Addr)*>(reloc) = module_id;
        }
        break;
      case R_GENERIC_TLS_DTPREL:
        count_relocation(kRelocRelative);
        MARK(rel->r_offset);
        TRACE_TYPE(RELO, "RELO TLS_DTPREL %16p <- %16p %s\n",
                   reinterpret_cast<void*>(reloc),
                   reinterpret_cast<void*>(sym_addr + addend), sym_name);
        *reinterpret_cast<ElfW(Addr)*>(reloc) = sym_addr + addend;
        break;
#endif  // !defined(__aarch64__)

#if defined(__aarch64__)
      // Bionic currently only implements TLSDESC for arm64. This implementation should work with
      // other architectures, as long as the resolver functions are implemented.
      case R_GENERIC_TLSDESC:
        count_relocation(kRelocRelative);
        MARK(rel->r_offset);
        {
          TlsDescriptor* desc = reinterpret_cast<TlsDescriptor*>(reloc);
          if (lsi == nullptr) {
            // Unresolved weak relocation.
            desc->func = tlsdesc_resolver_unresolved_weak;
            desc->arg = addend;
            TRACE_TYPE(RELO, "RELO TLSDESC %16p <- unresolved weak 0x%zx %s\n",
                       reinterpret_cast<void*>(reloc), static_cast<size_t>(addend), sym_name);
          } else {
            CHECK(lsi->get_tls() != nullptr); // We rejected a missing TLS segment above.
            size_t module_id = lsi->get_tls()->module_id;
            const TlsModule& mod = get_tls_module(module_id);
            if (mod.static_offset != SIZE_MAX) {
              desc->func = tlsdesc_resolver_static;
              desc->arg = mod.static_offset - tls_tp_base + sym_addr + addend;
              TRACE_TYPE(RELO, "RELO TLSDESC %16p <- static (0x%zx - 0x%zx + 0x%zx + 0x%zx) %s\n",
                         reinterpret_cast<void*>(reloc), mod.static_offset, tls_tp_base,
                         static_cast<size_t>(sym_addr), static_cast<size_t>(addend), sym_name);
            } else {
              tlsdesc_args_.push_back({
                .generation = mod.first_generation,
                .index.module_id = module_id,
                .index.offset = sym_addr + addend,
              });
              // Defer the TLSDESC relocation until the address of the TlsDynamicResolverArg object
              // is finalized.
              deferred_tlsdesc_relocs.push_back({ desc, tlsdesc_args_.size() - 1 });
              const TlsDynamicResolverArg& desc_arg = tlsdesc_args_.back();
              TRACE_TYPE(RELO, "RELO TLSDESC %16p <- dynamic (gen %zu, mod %zu, off %zu) %s",
                         reinterpret_cast<void*>(reloc), desc_arg.generation,
                         desc_arg.index.module_id, desc_arg.index.offset, sym_name);
            }
          }
        }
        break;
#endif  // defined(__aarch64__)

#if defined(__aarch64__)
      case R_AARCH64_ABS64:
        count_relocation(kRelocAbsolute);
        MARK(rel->r_offset);
        TRACE_TYPE(RELO, "RELO ABS64 %16llx <- %16llx %s\n",
                   reloc, sym_addr + addend, sym_name);
        *reinterpret_cast<ElfW(Addr)*>(reloc) = sym_addr + addend;
        break;
      case R_AARCH64_ABS32:
        count_relocation(kRelocAbsolute);
        MARK(rel->r_offset);
        TRACE_TYPE(RELO, "RELO ABS32 %16llx <- %16llx %s\n",
                   reloc, sym_addr + addend, sym_name);
        {
          const ElfW(Addr) min_value = static_cast<ElfW(Addr)>(INT32_MIN);
          const ElfW(Addr) max_value = static_cast<ElfW(Addr)>(UINT32_MAX);
          if ((min_value <= (sym_addr + addend)) &&
              ((sym_addr + addend) <= max_value)) {
            *reinterpret_cast<ElfW(Addr)*>(reloc) = sym_addr + addend;
          } else {
            DL_ERR("0x%016llx out of range 0x%016llx to 0x%016llx",
                   sym_addr + addend, min_value, max_value);
            return false;
          }
        }
        break;
      case R_AARCH64_ABS16:
        count_relocation(kRelocAbsolute);
        MARK(rel->r_offset);
        TRACE_TYPE(RELO, "RELO ABS16 %16llx <- %16llx %s\n",
                   reloc, sym_addr + addend, sym_name);
        {
          const ElfW(Addr) min_value = static_cast<ElfW(Addr)>(INT16_MIN);
          const ElfW(Addr) max_value = static_cast<ElfW(Addr)>(UINT16_MAX);
          if ((min_value <= (sym_addr + addend)) &&
              ((sym_addr + addend) <= max_value)) {
            *reinterpret_cast<ElfW(Addr)*>(reloc) = (sym_addr + addend);
          } else {
            DL_ERR("0x%016llx out of range 0x%016llx to 0x%016llx",
                   sym_addr + addend, min_value, max_value);
            return false;
          }
        }
        break;
      case R_AARCH64_PREL64:
        count_relocation(kRelocRelative);
        MARK(rel->r_offset);
        TRACE_TYPE(RELO, "RELO REL64 %16llx <- %16llx - %16llx %s\n",
                   reloc, sym_addr + addend, rel->r_offset, sym_name);
        *reinterpret_cast<ElfW(Addr)*>(reloc) = sym_addr + addend - rel->r_offset;
        break;
      case R_AARCH64_PREL32:
        count_relocation(kRelocRelative);
        MARK(rel->r_offset);
        TRACE_TYPE(RELO, "RELO REL32 %16llx <- %16llx - %16llx %s\n",
                   reloc, sym_addr + addend, rel->r_offset, sym_name);
        {
          const ElfW(Addr) min_value = static_cast<ElfW(Addr)>(INT32_MIN);
          const ElfW(Addr) max_value = static_cast<ElfW(Addr)>(UINT32_MAX);
          if ((min_value <= (sym_addr + addend - rel->r_offset)) &&
              ((sym_addr + addend - rel->r_offset) <= max_value)) {
            *reinterpret_cast<ElfW(Addr)*>(reloc) = sym_addr + addend - rel->r_offset;
          } else {
            DL_ERR("0x%016llx out of range 0x%016llx to 0x%016llx",
                   sym_addr + addend - rel->r_offset, min_value, max_value);
            return false;
          }
        }
        break;
      case R_AARCH64_PREL16:
        count_relocation(kRelocRelative);
        MARK(rel->r_offset);
        TRACE_TYPE(RELO, "RELO REL16 %16llx <- %16llx - %16llx %s\n",
                   reloc, sym_addr + addend, rel->r_offset, sym_name);
        {
          const ElfW(Addr) min_value = static_cast<ElfW(Addr)>(INT16_MIN);
          const ElfW(Addr) max_value = static_cast<ElfW(Addr)>(UINT16_MAX);
          if ((min_value <= (sym_addr + addend - rel->r_offset)) &&
              ((sym_addr + addend - rel->r_offset) <= max_value)) {
            *reinterpret_cast<ElfW(Addr)*>(reloc) = sym_addr + addend - rel->r_offset;
          } else {
            DL_ERR("0x%016llx out of range 0x%016llx to 0x%016llx",
                   sym_addr + addend - rel->r_offset, min_value, max_value);
            return false;
          }
        }
        break;

      case R_AARCH64_COPY:
        /*
         * ET_EXEC is not supported so this should not happen.
         *
         * http://infocenter.arm.com/help/topic/com.arm.doc.ihi0056b/IHI0056B_aaelf64.pdf
         *
         * Section 4.6.11 "Dynamic relocations"
         * R_AARCH64_COPY may only appear in executable objects where e_type is
         * set to ET_EXEC.
         */
        DL_ERR("%s R_AARCH64_COPY relocations are not supported", get_realpath());
        return false;
#elif defined(__x86_64__)
      case R_X86_64_32:
        count_relocation(kRelocRelative);
        MARK(rel->r_offset);
        TRACE_TYPE(RELO, "RELO R_X86_64_32 %08zx <- +%08zx %s", static_cast<size_t>(reloc),
                   static_cast<size_t>(sym_addr), sym_name);
        *reinterpret_cast<Elf32_Addr*>(reloc) = sym_addr + addend;
        break;
      case R_X86_64_64:
        count_relocation(kRelocRelative);
        MARK(rel->r_offset);
        TRACE_TYPE(RELO, "RELO R_X86_64_64 %08zx <- +%08zx %s", static_cast<size_t>(reloc),
                   static_cast<size_t>(sym_addr), sym_name);
        *reinterpret_cast<Elf64_Addr*>(reloc) = sym_addr + addend;
        break;
      case R_X86_64_PC32:
        count_relocation(kRelocRelative);
        MARK(rel->r_offset);
        TRACE_TYPE(RELO, "RELO R_X86_64_PC32 %08zx <- +%08zx (%08zx - %08zx) %s",
                   static_cast<size_t>(reloc), static_cast<size_t>(sym_addr - reloc),
                   static_cast<size_t>(sym_addr), static_cast<size_t>(reloc), sym_name);
        *reinterpret_cast<Elf32_Addr*>(reloc) = sym_addr + addend - reloc;
        break;
#elif defined(__arm__)
      case R_ARM_ABS32:
        count_relocation(kRelocAbsolute);
        MARK(rel->r_offset);
        TRACE_TYPE(RELO, "RELO ABS %08x <- %08x %s", reloc, sym_addr, sym_name);
        *reinterpret_cast<ElfW(Addr)*>(reloc) += sym_addr;
        break;
      case R_ARM_REL32:
        count_relocation(kRelocRelative);
        MARK(rel->r_offset);
        TRACE_TYPE(RELO, "RELO REL32 %08x <- %08x - %08x %s",
                   reloc, sym_addr, rel->r_offset, sym_name);
        *reinterpret_cast<ElfW(Addr)*>(reloc) += sym_addr - rel->r_offset;
        break;
      case R_ARM_COPY:
        /*
         * ET_EXEC is not supported so this should not happen.
         *
         * http://infocenter.arm.com/help/topic/com.arm.doc.ihi0044d/IHI0044D_aaelf.pdf
         *
         * Section 4.6.1.10 "Dynamic relocations"
         * R_ARM_COPY may only appear in executable objects where e_type is
         * set to ET_EXEC.
         */
        DL_ERR("%s R_ARM_COPY relocations are not supported", get_realpath());
        return false;
#elif defined(__i386__)
      case R_386_32:
        count_relocation(kRelocRelative);
        MARK(rel->r_offset);
        TRACE_TYPE(RELO, "RELO R_386_32 %08x <- +%08x %s", reloc, sym_addr, sym_name);
        *reinterpret_cast<ElfW(Addr)*>(reloc) += sym_addr;
        break;
      case R_386_PC32:
        count_relocation(kRelocRelative);
        MARK(rel->r_offset);
        TRACE_TYPE(RELO, "RELO R_386_PC32 %08x <- +%08x (%08x - %08x) %s",
                   reloc, (sym_addr - reloc), sym_addr, reloc, sym_name);
        *reinterpret_cast<ElfW(Addr)*>(reloc) += (sym_addr - reloc);
        break;
#endif
      default:
        DL_ERR("unknown reloc type %d @ %p (%zu)", type, rel, idx);
        return false;
    }
  }

#if defined(__aarch64__)
  // Bionic currently only implements TLSDESC for arm64.
  for (const std::pair<TlsDescriptor*, size_t>& pair : deferred_tlsdesc_relocs) {
    TlsDescriptor* desc = pair.first;
    desc->func = tlsdesc_resolver_dynamic;
    desc->arg = reinterpret_cast<size_t>(&tlsdesc_args_[pair.second]);
  }
#endif

  return true;
}
#endif  // !defined(__mips__)

// An empty list of soinfos
static soinfo_list_t g_empty_list;

bool soinfo::prelink_image() {
  /* Extract dynamic section */
  ElfW(Word) dynamic_flags = 0;
  phdr_table_get_dynamic_section(phdr, phnum, load_bias, &dynamic, &dynamic_flags);

  /* We can't log anything until the linker is relocated */
  bool relocating_linker = (flags_ & FLAG_LINKER) != 0;
  if (!relocating_linker) {
    INFO("[ Linking \"%s\" ]", get_realpath());
    DEBUG("si->base = %p si->flags = 0x%08x", reinterpret_cast<void*>(base), flags_);
  }

  if (dynamic == nullptr) {
    if (!relocating_linker) {
      DL_ERR("missing PT_DYNAMIC in \"%s\"", get_realpath());
    }
    return false;
  } else {
    if (!relocating_linker) {
      DEBUG("dynamic = %p", dynamic);
    }
  }

#if defined(__arm__)
  (void) phdr_table_get_arm_exidx(phdr, phnum, load_bias,
                                  &ARM_exidx, &ARM_exidx_count);
#endif

  /*TlsSegment tls_segment;
  if (__bionic_get_tls_segment(phdr, phnum, load_bias, &tls_segment)) {
    if (!__bionic_check_tls_alignment(&tls_segment.alignment)) {
      if (!relocating_linker) {
        DL_ERR("TLS segment alignment in \"%s\" is not a power of 2: %zu",
               get_realpath(), tls_segment.alignment);
      }
      return false;
    }
    tls_ = std::make_unique<soinfo_tls>();
    tls_->segment = tls_segment;
  }*/

  // Extract useful information from dynamic section.
  // Note that: "Except for the DT_NULL element at the end of the array,
  // and the relative order of DT_NEEDED elements, entries may appear in any order."
  //
  // source: http://www.sco.com/developers/gabi/1998-04-29/ch5.dynamic.html
  uint32_t needed_count = 0;
  ElfW(Addr) base;
  for (ElfW(Dyn)* d = dynamic; d->d_tag != DT_NULL; ++d) {
    DEBUG("d = %p, d[0](tag) = %p d[1](val) = %p",
          d, reinterpret_cast<void*>(d->d_tag), reinterpret_cast<void*>(d->d_un.d_val));
    switch (d->d_tag) {
      case DT_SONAME:
        // this is parsed after we have strtab initialized (see below).
        break;

      case DT_HASH:
        if(d->d_un.d_ptr > load_bias)
          base = d->d_un.d_ptr;
        else
          base = load_bias + d->d_un.d_ptr;

        nbucket_ = reinterpret_cast<uint32_t*>(base)[0];
        nchain_ = reinterpret_cast<uint32_t*>(base)[1];
        bucket_ = reinterpret_cast<uint32_t*>(base + 8);
        chain_ = reinterpret_cast<uint32_t*>(base+ 8 + nbucket_ * 4);
        break;

      case DT_GNU_HASH:
        if(d->d_un.d_ptr > load_bias)
          base = d->d_un.d_ptr;
        else
          base = load_bias + d->d_un.d_ptr;
        
        gnu_nbucket_ = reinterpret_cast<uint32_t*>(base)[0];
        // skip symndx
        gnu_maskwords_ = reinterpret_cast<uint32_t*>(base)[2];
        gnu_shift2_ = reinterpret_cast<uint32_t*>(base)[3];

        gnu_bloom_filter_ = reinterpret_cast<ElfW(Addr)*>(base + 16);
        gnu_bucket_ = reinterpret_cast<uint32_t*>(gnu_bloom_filter_ + gnu_maskwords_);
        // amend chain for symndx = header[1]
        gnu_chain_ = gnu_bucket_ + gnu_nbucket_ -
            reinterpret_cast<uint32_t*>(base)[1];

        if (!powerof2(gnu_maskwords_)) {
          DL_ERR("invalid maskwords for gnu_hash = 0x%x, in \"%s\" expecting power to two",
              gnu_maskwords_, get_realpath());
          return false;
        }
        --gnu_maskwords_;

        flags_ |= FLAG_GNU_HASH;
        break;

      case DT_STRTAB:
        strtab_ = reinterpret_cast<const char*>(load_bias + d->d_un.d_ptr);
        break;

      case DT_STRSZ:
        strtab_size_ = d->d_un.d_val;
        break;

      case DT_SYMTAB:
        symtab_ = reinterpret_cast<ElfW(Sym)*>(load_bias + d->d_un.d_ptr);
        break;

      case DT_SYMENT:
        if (d->d_un.d_val != sizeof(ElfW(Sym))) {
          DL_ERR("invalid DT_SYMENT: %zd in \"%s\"",
              static_cast<size_t>(d->d_un.d_val), get_realpath());
          return false;
        }
        break;

      case DT_PLTREL:
#if defined(USE_RELA)
        if (d->d_un.d_val != DT_RELA) {
          DL_ERR("unsupported DT_PLTREL in \"%s\"; expected DT_RELA", get_realpath());
          return false;
        }
#else
        if (d->d_un.d_val != DT_REL) {
          DL_ERR("unsupported DT_PLTREL in \"%s\"; expected DT_REL", get_realpath());
          return false;
        }
#endif
        break;

      case DT_JMPREL:
#if defined(USE_RELA)
        plt_rela_ = reinterpret_cast<ElfW(Rela)*>(load_bias + d->d_un.d_ptr);
#else
        plt_rel_ = reinterpret_cast<ElfW(Rel)*>(load_bias + d->d_un.d_ptr);
#endif
        break;

      case DT_PLTRELSZ:
#if defined(USE_RELA)
        plt_rela_count_ = d->d_un.d_val / sizeof(ElfW(Rela));
#else
        plt_rel_count_ = d->d_un.d_val / sizeof(ElfW(Rel));
#endif
        break;

      case DT_PLTGOT:
#if defined(__mips__)
        // Used by mips and mips64.
        plt_got_ = reinterpret_cast<ElfW(Addr)**>(load_bias + d->d_un.d_ptr);
#endif
        // Ignore for other platforms... (because RTLD_LAZY is not supported)
        break;

      case DT_DEBUG:
        // Set the DT_DEBUG entry to the address of _r_debug for GDB
        // if the dynamic table is writable
// FIXME: not working currently for N64
// The flags for the LOAD and DYNAMIC program headers do not agree.
// The LOAD section containing the dynamic table has been mapped as
// read-only, but the DYNAMIC header claims it is writable.
#if !(defined(__mips__) && defined(__LP64__))
        if ((dynamic_flags & PF_W) != 0) {
          d->d_un.d_val = reinterpret_cast<uintptr_t>(&_r_debug);
        }
#endif
        break;
#if defined(USE_RELA)
      case DT_RELA:
        rela_ = reinterpret_cast<ElfW(Rela)*>(load_bias + d->d_un.d_ptr);
        break;

      case DT_RELASZ:
        rela_count_ = d->d_un.d_val / sizeof(ElfW(Rela));
        break;

      case DT_ANDROID_RELA:
        android_relocs_ = reinterpret_cast<uint8_t*>(load_bias + d->d_un.d_ptr);
        break;

      case DT_ANDROID_RELASZ:
        android_relocs_size_ = d->d_un.d_val;
        break;

      case DT_ANDROID_REL:
        DL_ERR("unsupported DT_ANDROID_REL in \"%s\"", get_realpath());
        return false;

      case DT_ANDROID_RELSZ:
        DL_ERR("unsupported DT_ANDROID_RELSZ in \"%s\"", get_realpath());
        return false;

      case DT_RELAENT:
        if (d->d_un.d_val != sizeof(ElfW(Rela))) {
          DL_ERR("invalid DT_RELAENT: %zd", static_cast<size_t>(d->d_un.d_val));
          return false;
        }
        break;

      // Ignored (see DT_RELCOUNT comments for details).
      case DT_RELACOUNT:
        break;

      case DT_REL:
        DL_ERR("unsupported DT_REL in \"%s\"", get_realpath());
        return false;

      case DT_RELSZ:
        DL_ERR("unsupported DT_RELSZ in \"%s\"", get_realpath());
        return false;

#else
      case DT_REL:
        rel_ = reinterpret_cast<ElfW(Rel)*>(load_bias + d->d_un.d_ptr);
        break;

      case DT_RELSZ:
        rel_count_ = d->d_un.d_val / sizeof(ElfW(Rel));
        break;

      case DT_RELENT:
        if (d->d_un.d_val != sizeof(ElfW(Rel))) {
          DL_ERR("invalid DT_RELENT: %zd", static_cast<size_t>(d->d_un.d_val));
          return false;
        }
        break;

      case DT_ANDROID_REL:
        android_relocs_ = reinterpret_cast<uint8_t*>(load_bias + d->d_un.d_ptr);
        break;

      case DT_ANDROID_RELSZ:
        android_relocs_size_ = d->d_un.d_val;
        break;

      case DT_ANDROID_RELA:
        DL_ERR("unsupported DT_ANDROID_RELA in \"%s\"", get_realpath());
        return false;

      case DT_ANDROID_RELASZ:
        DL_ERR("unsupported DT_ANDROID_RELASZ in \"%s\"", get_realpath());
        return false;

      // "Indicates that all RELATIVE relocations have been concatenated together,
      // and specifies the RELATIVE relocation count."
      //
      // TODO: Spec also mentions that this can be used to optimize relocation process;
      // Not currently used by bionic linker - ignored.
      case DT_RELCOUNT:
        break;

      case DT_RELA:
        DL_ERR("unsupported DT_RELA in \"%s\"", get_realpath());
        return false;

      case DT_RELASZ:
        DL_ERR("unsupported DT_RELASZ in \"%s\"", get_realpath());
        return false;

#endif
      case DT_RELR:
        relr_ = reinterpret_cast<ElfW(Relr)*>(load_bias + d->d_un.d_ptr);
        break;

      case DT_RELRSZ:
        relr_count_ = d->d_un.d_val / sizeof(ElfW(Relr));
        break;

      case DT_RELRENT:
        if (d->d_un.d_val != sizeof(ElfW(Relr))) {
          DL_ERR("invalid DT_RELRENT: %zd", static_cast<size_t>(d->d_un.d_val));
          return false;
        }
        break;

      // Ignored (see DT_RELCOUNT comments for details).
      case DT_RELRCOUNT:
        break;

      case DT_INIT:
        init_func_ = reinterpret_cast<linker_ctor_function_t>(load_bias + d->d_un.d_ptr);
        DEBUG("%s constructors (DT_INIT) found at %p", get_realpath(), init_func_);
        break;

      case DT_FINI:
        fini_func_ = reinterpret_cast<linker_dtor_function_t>(load_bias + d->d_un.d_ptr);
        DEBUG("%s destructors (DT_FINI) found at %p", get_realpath(), fini_func_);
        break;

      case DT_INIT_ARRAY:
        init_array_ = reinterpret_cast<linker_ctor_function_t*>(load_bias + d->d_un.d_ptr);
        DEBUG("%s constructors (DT_INIT_ARRAY) found at %p", get_realpath(), init_array_);
        break;

      case DT_INIT_ARRAYSZ:
        init_array_count_ = static_cast<uint32_t>(d->d_un.d_val) / sizeof(ElfW(Addr));
        break;

      case DT_FINI_ARRAY:
        fini_array_ = reinterpret_cast<linker_dtor_function_t*>(load_bias + d->d_un.d_ptr);
        DEBUG("%s destructors (DT_FINI_ARRAY) found at %p", get_realpath(), fini_array_);
        break;

      case DT_FINI_ARRAYSZ:
        fini_array_count_ = static_cast<uint32_t>(d->d_un.d_val) / sizeof(ElfW(Addr));
        break;

      case DT_PREINIT_ARRAY:
        preinit_array_ = reinterpret_cast<linker_ctor_function_t*>(load_bias + d->d_un.d_ptr);
        DEBUG("%s constructors (DT_PREINIT_ARRAY) found at %p", get_realpath(), preinit_array_);
        break;

      case DT_PREINIT_ARRAYSZ:
        preinit_array_count_ = static_cast<uint32_t>(d->d_un.d_val) / sizeof(ElfW(Addr));
        break;

      case DT_TEXTREL:
#if defined(__LP64__)
        DL_ERR("\"%s\" has text relocations", get_realpath());
        return false;
#else
        has_text_relocations = true;
        break;
#endif

      case DT_SYMBOLIC:
        has_DT_SYMBOLIC = true;
        break;

      case DT_NEEDED:
        ++needed_count;
        break;

      case DT_FLAGS:
        if (d->d_un.d_val & DF_TEXTREL) {
#if defined(__LP64__)
          DL_ERR("\"%s\" has text relocations", get_realpath());
          return false;
#else
          has_text_relocations = true;
#endif
        }
        if (d->d_un.d_val & DF_SYMBOLIC) {
          has_DT_SYMBOLIC = true;
        }
        break;

      case DT_FLAGS_1:
        set_dt_flags_1(d->d_un.d_val);

        if ((d->d_un.d_val & ~SUPPORTED_DT_FLAGS_1) != 0) {
          DL_WARN("Warning: \"%s\" has unsupported flags DT_FLAGS_1=%p "
                  "(ignoring unsupported flags)",
                  get_realpath(), reinterpret_cast<void*>(d->d_un.d_val));
        }
        break;
#if defined(__mips__)
      case DT_MIPS_RLD_MAP:
        // Set the DT_MIPS_RLD_MAP entry to the address of _r_debug for GDB.
        {
          r_debug** dp = reinterpret_cast<r_debug**>(load_bias + d->d_un.d_ptr);
          *dp = &_r_debug;
        }
        break;
      case DT_MIPS_RLD_MAP_REL:
        // Set the DT_MIPS_RLD_MAP_REL entry to the address of _r_debug for GDB.
        {
          r_debug** dp = reinterpret_cast<r_debug**>(
              reinterpret_cast<ElfW(Addr)>(d) + d->d_un.d_val);
          *dp = &_r_debug;
        }
        break;

      case DT_MIPS_RLD_VERSION:
      case DT_MIPS_FLAGS:
      case DT_MIPS_BASE_ADDRESS:
      case DT_MIPS_UNREFEXTNO:
        break;

      case DT_MIPS_SYMTABNO:
        mips_symtabno_ = d->d_un.d_val;
        break;

      case DT_MIPS_LOCAL_GOTNO:
        mips_local_gotno_ = d->d_un.d_val;
        break;

      case DT_MIPS_GOTSYM:
        mips_gotsym_ = d->d_un.d_val;
        break;
#endif
      // Ignored: "Its use has been superseded by the DF_BIND_NOW flag"
      case DT_BIND_NOW:
        break;

      case DT_VERSYM:
        versym_ = reinterpret_cast<ElfW(Versym)*>(load_bias + d->d_un.d_ptr);
        break;

      case DT_VERDEF:
        verdef_ptr_ = load_bias + d->d_un.d_ptr;
        break;
      case DT_VERDEFNUM:
        verdef_cnt_ = d->d_un.d_val;
        break;

      case DT_VERNEED:
        verneed_ptr_ = load_bias + d->d_un.d_ptr;
        break;

      case DT_VERNEEDNUM:
        verneed_cnt_ = d->d_un.d_val;
        break;

      case DT_RUNPATH:
        // this is parsed after we have strtab initialized (see below).
        break;

      case DT_TLSDESC_GOT:
      case DT_TLSDESC_PLT:
        // These DT entries are used for lazy TLSDESC relocations. Bionic
        // resolves everything eagerly, so these can be ignored.
        break;

      default:
        if (!relocating_linker) {
          const char* tag_name;
          if (d->d_tag == DT_RPATH) {
            tag_name = "DT_RPATH";
          } else if (d->d_tag == DT_ENCODING) {
            tag_name = "DT_ENCODING";
          } else if (d->d_tag >= DT_LOOS && d->d_tag <= DT_HIOS) {
            tag_name = "unknown OS-specific";
          } else if (d->d_tag >= DT_LOPROC && d->d_tag <= DT_HIPROC) {
            tag_name = "unknown processor-specific";
          } else {
            tag_name = "unknown";
          }
          DL_WARN("Warning: \"%s\" unused DT entry: %s (type %p arg %p) (ignoring)",
                  get_realpath(),
                  tag_name,
                  reinterpret_cast<void*>(d->d_tag),
                  reinterpret_cast<void*>(d->d_un.d_val));
        }
        break;
    }
  }

#if defined(__mips__) && !defined(__LP64__)
  if (!mips_check_and_adjust_fp_modes()) {
    return false;
  }
#endif

  DEBUG("si->base = %p, si->strtab = %p, si->symtab = %p",
        reinterpret_cast<void*>(base), strtab_, symtab_);

  // Sanity checks.
  if (relocating_linker && needed_count != 0) {
    DL_WARN("linker cannot have DT_NEEDED dependencies on other libraries");
    //return false;
  }
  if (nbucket_ == 0 && gnu_nbucket_ == 0) {
    DL_ERR("empty/missing DT_HASH/DT_GNU_HASH in \"%s\" "
        "(new hash type from the future?)", get_realpath());
    return false;
  }
  if (strtab_ == nullptr) {
    DL_ERR("empty/missing DT_STRTAB in \"%s\"", get_realpath());
    return false;
  }
  if (symtab_ == nullptr) {
    DL_ERR("empty/missing DT_SYMTAB in \"%s\"", get_realpath());
    return false;
  }

  // second pass - parse entries relying on strtab
  for (ElfW(Dyn)* d = dynamic; d->d_tag != DT_NULL; ++d) {
    switch (d->d_tag) {
      case DT_SONAME:
        set_soname(get_string(d->d_un.d_val));
        break;
      case DT_RUNPATH:
        set_dt_runpath(get_string(d->d_un.d_val));
        break;
    }
  }

  // Before M release linker was using basename in place of soname.
  // In the case when dt_soname is absent some apps stop working
  // because they can't find dt_needed library by soname.
  // This workaround should keep them working. (Applies only
  // for apps targeting sdk version < M.) Make an exception for
  // the main executable and linker; they do not need to have dt_soname.
  // TODO: >= O the linker doesn't need this workaround.
  if (soname_ == nullptr &&
      this != solist_get_somain() &&
      (flags_ & FLAG_LINKER) == 0 &&
      get_application_target_sdk_version() < __ANDROID_API_M__) {
    soname_ = basename(realpath_.c_str());
    DL_WARN_documented_change(__ANDROID_API_M__,
                              "missing-soname-enforced-for-api-level-23",
                              "\"%s\" has no DT_SONAME (will use %s instead)",
                              get_realpath(), soname_);

    // Don't call add_dlwarning because a missing DT_SONAME isn't important enough to show in the UI
  }
  return true;
}

bool soinfo::link_image(const soinfo_list_t& global_group, const soinfo_list_t& local_group,
                        const android_dlextinfo* extinfo, size_t* relro_fd_offset) {
  if (is_image_linked()) {
    // already linked.
    return true;
  }

  local_group_root_ = local_group.front();
  if (local_group_root_ == nullptr) {
    local_group_root_ = this;
  }

  if ((flags_ & FLAG_LINKER) == 0 && local_group_root_ == this) {
    target_sdk_version_ = get_application_target_sdk_version();
  }

  VersionTracker version_tracker;

  if (!version_tracker.init(this)) {
    return false;
  }

#if !defined(__LP64__)
  if (has_text_relocations) {
    // Fail if app is targeting M or above.
    int app_target_api_level = get_application_target_sdk_version();
    if (app_target_api_level >= __ANDROID_API_M__) {
      DL_ERR_AND_LOG("\"%s\" has text relocations (https://android.googlesource.com/platform/"
                     "bionic/+/master/android-changes-for-ndk-developers.md#Text-Relocations-"
                     "Enforced-for-API-level-23)", get_realpath());
      return false;
    }
    // Make segments writable to allow text relocations to work properly. We will later call
    // phdr_table_protect_segments() after all of them are applied.
    DL_WARN_documented_change(__ANDROID_API_M__,
                              "Text-Relocations-Enforced-for-API-level-23",
                              "\"%s\" has text relocations",
                              get_realpath());
    add_dlwarning(get_realpath(), "text relocations");
    if (phdr_table_unprotect_segments(phdr, phnum, load_bias) < 0) {
      DL_ERR("can't unprotect loadable segments for \"%s\": %s", get_realpath(), strerror(errno));
      return false;
    }
  }
#endif

  if (android_relocs_ != nullptr) {
    // check signature
    if (android_relocs_size_ > 3 &&
        android_relocs_[0] == 'A' &&
        android_relocs_[1] == 'P' &&
        android_relocs_[2] == 'S' &&
        android_relocs_[3] == '2') {
      DEBUG("[ android relocating %s ]", get_realpath());

      bool relocated = false;
      const uint8_t* packed_relocs = android_relocs_ + 4;
      const size_t packed_relocs_size = android_relocs_size_ - 4;

      relocated = relocate(
          version_tracker,
          packed_reloc_iterator<sleb128_decoder>(
            sleb128_decoder(packed_relocs, packed_relocs_size)),
          global_group, local_group);

      if (!relocated) {
        return false;
      }
    } else {
      DL_ERR("bad android relocation header.");
      return false;
    }
  }

  if (relr_ != nullptr) {
    DEBUG("[ relocating %s relr ]", get_realpath());
    if (!relocate_relr()) {
      return false;
    }
  }

#if defined(USE_RELA)
  if (rela_ != nullptr) {
    DEBUG("[ relocating %s rela ]", get_realpath());
    if (!relocate(version_tracker,
            plain_reloc_iterator(rela_, rela_count_), global_group, local_group)) {
      return false;
    }
  }
  if (plt_rela_ != nullptr) {
    DEBUG("[ relocating %s plt rela ]", get_realpath());
    if (!relocate(version_tracker,
            plain_reloc_iterator(plt_rela_, plt_rela_count_), global_group, local_group)) {
      return false;
    }
  }
#else
  if (rel_ != nullptr) {
    DEBUG("[ relocating %s rel ]", get_realpath());
    if (!relocate(version_tracker,
            plain_reloc_iterator(rel_, rel_count_), global_group, local_group)) {
      return false;
    }
  }
  if (plt_rel_ != nullptr) {
    DEBUG("[ relocating %s plt rel ]", get_realpath());
    if (!relocate(version_tracker,
            plain_reloc_iterator(plt_rel_, plt_rel_count_), global_group, local_group)) {
      return false;
    }
  }
#endif

#if defined(__mips__)
  if (!mips_relocate_got(version_tracker, global_group, local_group)) {
    return false;
  }
#endif

  DEBUG("[ finished linking %s ]", get_realpath());

#if !defined(__LP64__)
  if (has_text_relocations) {
    // All relocations are done, we can protect our segments back to read-only.
    if (phdr_table_protect_segments(phdr, phnum, load_bias) < 0) {
      DL_ERR("can't protect segments for \"%s\": %s",
             get_realpath(), strerror(errno));
      return false;
    }
  }
#endif

  // We can also turn on GNU RELRO protection if we're not linking the dynamic linker
  // itself --- it can't make system calls yet, and will have to call protect_relro later.
  if (!is_linker() && !protect_relro()) {
    return false;
  }

  /* Handle serializing/sharing the RELRO segment */
  if (extinfo && (extinfo->flags & ANDROID_DLEXT_WRITE_RELRO)) {
    if (phdr_table_serialize_gnu_relro(phdr, phnum, load_bias,
                                       extinfo->relro_fd, relro_fd_offset) < 0) {
      DL_ERR("failed serializing GNU RELRO section for \"%s\": %s",
             get_realpath(), strerror(errno));
      return false;
    }
  } else if (extinfo && (extinfo->flags & ANDROID_DLEXT_USE_RELRO)) {
    if (phdr_table_map_gnu_relro(phdr, phnum, load_bias,
                                 extinfo->relro_fd, relro_fd_offset) < 0) {
      DL_ERR("failed mapping GNU RELRO section for \"%s\": %s",
             get_realpath(), strerror(errno));
      return false;
    }
  }

  notify_gdb_of_load(this);
  set_image_linked();
  return true;
}

bool soinfo::protect_relro() {
  if (phdr_table_protect_gnu_relro(phdr, phnum, load_bias) < 0) {
    DL_ERR("can't enable GNU RELRO protection for \"%s\": %s",
           get_realpath(), strerror(errno));
    return false;
  }
  return true;
}

static std::vector<android_namespace_t*> init_default_namespace_no_config(bool is_asan) {
  g_default_namespace->set_isolated(false);
  auto default_ld_paths = is_asan ? kAsanDefaultLdPaths : kDefaultLdPaths;

  char real_path[PATH_MAX];
  std::vector<std::string> ld_default_paths;
  for (size_t i = 0; default_ld_paths[i] != nullptr; ++i) {
    if (realpath(default_ld_paths[i], real_path) != nullptr) {
      ld_default_paths.push_back(real_path);
    } else {
      ld_default_paths.push_back(default_ld_paths[i]);
    }
  }

  g_default_namespace->set_default_library_paths(std::move(ld_default_paths));

  std::vector<android_namespace_t*> namespaces;
  namespaces.push_back(g_default_namespace);
  return namespaces;
}

// return /apex/<name>/etc/ld.config.txt from /apex/<name>/bin/<exec>
static std::string get_ld_config_file_apex_path(const char* executable_path) {
  std::vector<std::string> paths = split(executable_path, "/");
  if (paths.size() == 5 && paths[1] == "apex" && paths[3] == "bin") {
    return std::string("/apex/") + paths[2] + "/etc/ld.config.txt";
  }
  return "";
}

static std::string get_ld_config_file_vndk_path() {
  /*if (android::base::GetBoolProperty("ro.vndk.lite", false)) {
    return kLdConfigVndkLiteFilePath;
  }*/

  std::string ld_config_file_vndk = kLdConfigFilePath;
  size_t insert_pos = ld_config_file_vndk.find_last_of('.');
  if (insert_pos == std::string::npos) {
    insert_pos = ld_config_file_vndk.length();
  }
  ld_config_file_vndk.insert(insert_pos, Config::get_vndk_version_string('.'));
  return ld_config_file_vndk;
}

static std::string get_ld_config_file_path(const char* executable_path) {
#ifdef USE_LD_CONFIG_FILE
  // This is a debugging/testing only feature. Must not be available on
  // production builds.
  const char* ld_config_file_env = getenv("LD_CONFIG_FILE");
  if (ld_config_file_env != nullptr && file_exists(ld_config_file_env)) {
    return ld_config_file_env;
  }
#endif

  std::string path = get_ld_config_file_apex_path(executable_path);
  if (!path.empty()) {
    if (file_exists(path.c_str())) {
      return path;
    }
    DL_WARN("Warning: couldn't read config file \"%s\" for \"%s\"",
            path.c_str(), executable_path);
  }

  path = kLdConfigArchFilePath;
  if (file_exists(path.c_str())) {
    return path;
  }

  path = get_ld_config_file_vndk_path();
  if (file_exists(path.c_str())) {
    return path;
  }

  return kLdConfigFilePath;
}

std::vector<android_namespace_t*> init_default_namespaces(const char* executable_path) {
  g_default_namespace->set_name("(default)");

#if DISABLED_FOR_HYBRIS_SUPPORT
  soinfo* somain = solist_get_somain();

  const char *interp = phdr_table_get_interpreter_name(somain->phdr, somain->phnum,
                                                       somain->load_bias);
  const char* bname = (interp != nullptr) ? basename(interp) : nullptr;

  g_is_asan = bname != nullptr &&
              (strcmp(bname, "linker_asan") == 0 ||
               strcmp(bname, "linker_asan64") == 0);
#endif

  const Config* config = nullptr;

  std::string error_msg;

  std::string ld_config_file_path = get_ld_config_file_path(executable_path);

  INFO("[ Reading linker config \"%s\" ]", ld_config_file_path.c_str());
  if (!Config::read_binary_config(ld_config_file_path.c_str(),
                                  executable_path,
                                  g_is_asan,
                                  &config,
                                  &error_msg)) {
    if (!error_msg.empty()) {
      DL_WARN("Warning: couldn't read \"%s\" for \"%s\" (using default configuration instead): %s",
              ld_config_file_path.c_str(),
              executable_path,
              error_msg.c_str());
    }
    config = nullptr;
  }

  if (config == nullptr) {
    return init_default_namespace_no_config(g_is_asan);
  }

  const auto& namespace_configs = config->namespace_configs();
  std::unordered_map<std::string, android_namespace_t*> namespaces;

  // 1. Initialize default namespace
  const NamespaceConfig* default_ns_config = config->default_namespace_config();

  g_default_namespace->set_isolated(default_ns_config->isolated());
  g_default_namespace->set_default_library_paths(default_ns_config->search_paths());
  g_default_namespace->set_permitted_paths(default_ns_config->permitted_paths());

  namespaces[default_ns_config->name()] = g_default_namespace;
  if (default_ns_config->visible()) {
    g_exported_namespaces[default_ns_config->name()] = g_default_namespace;
  }

  // 2. Initialize other namespaces

  for (auto& ns_config : namespace_configs) {
    if (namespaces.find(ns_config->name()) != namespaces.end()) {
      continue;
    }

    android_namespace_t* ns = new (g_namespace_allocator.alloc()) android_namespace_t();
    ns->set_name(ns_config->name());
    ns->set_isolated(ns_config->isolated());
    ns->set_default_library_paths(ns_config->search_paths());
    ns->set_permitted_paths(ns_config->permitted_paths());
    ns->set_whitelisted_libs(ns_config->whitelisted_libs());

    namespaces[ns_config->name()] = ns;
    if (ns_config->visible()) {
      g_exported_namespaces[ns_config->name()] = ns;
    }
  }

  // 3. Establish links between namespaces
  for (auto& ns_config : namespace_configs) {
    auto it_from = namespaces.find(ns_config->name());
    CHECK(it_from != namespaces.end());
    android_namespace_t* namespace_from = it_from->second;
    for (const NamespaceLinkConfig& ns_link : ns_config->links()) {
      auto it_to = namespaces.find(ns_link.ns_name());
      CHECK(it_to != namespaces.end());
      android_namespace_t* namespace_to = it_to->second;
      if (ns_link.allow_all_shared_libs()) {
        link_namespaces_all_libs(namespace_from, namespace_to);
      } else {
        link_namespaces(namespace_from, namespace_to, ns_link.shared_libs().c_str());
      }
    }
  }
  // we can no longer rely on the fact that libdl.so is part of default namespace
  // this is why we want to add ld-android.so to all namespaces from ld.config.txt
  soinfo* ld_android_so = solist_get_head();

  // we also need vdso to be available for all namespaces (if present)
  soinfo* vdso = solist_get_vdso();
  for (auto it : namespaces) {
    it.second->add_soinfo(ld_android_so);
    if (vdso != nullptr) {
      it.second->add_soinfo(vdso);
    }
    // somain and ld_preloads are added to these namespaces after LD_PRELOAD libs are linked
  }

  set_application_target_sdk_version(config->target_sdk_version());

  std::vector<android_namespace_t*> created_namespaces;
  created_namespaces.reserve(namespaces.size());
  for (const auto& kv : namespaces) {
    created_namespaces.push_back(kv.second);
  }
  return created_namespaces;
}

// This function finds a namespace exported in ld.config.txt by its name.
// A namespace can be exported by setting .visible property to true.
android_namespace_t* get_exported_namespace(const char* name) {
  if (name == nullptr) {
    return nullptr;
  }
  auto it = g_exported_namespaces.find(std::string(name));
  if (it == g_exported_namespaces.end()) {
    return nullptr;
  }
  return it->second;
}

void purge_unused_memory() {
  // For now, we only purge the memory used by LoadTask because we know those
  // are temporary objects.
  //
  // Purging other LinkerBlockAllocator hardly yields much because they hold
  // information about namespaces and opened libraries, which are not freed
  // when the control leaves the linker.
  //
  // Purging BionicAllocator may give us a few dirty pages back, but those pages
  // would be already zeroed out, so they compress easily in ZRAM.  Therefore,
  // it is not worth munmap()'ing those pages.
  TypeBasedAllocator<LoadTask>::purge();
}
