/*
 * Copyright (C) 2017 The Android Open Source Project
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

#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

#include <gtest/gtest.h>

#include "linker_config.h"
#include "linker_utils.h"

#include <unistd.h>

#include <android-base/file.h>
#include <android-base/scopeguard.h>
#include <android-base/stringprintf.h>

#if defined(__LP64__)
#define ARCH_SUFFIX "64"
#else
#define ARCH_SUFFIX ""
#endif

static const char* config_str =
  "# comment \n"
  "dir.test = /data/local/tmp\n"
  "\n"
  "[test]\n"
  "\n"
  "enable.target.sdk.version = true\n"
  "additional.namespaces=system\n"
  "additional.namespaces+=vndk\n"
  "additional.namespaces+=vndk_in_system\n"
  "namespace.default.isolated = true\n"
  "namespace.default.search.paths = /vendor/${LIB}\n"
  "namespace.default.permitted.paths = /vendor/${LIB}\n"
  "namespace.default.asan.search.paths = /data\n"
  "namespace.default.asan.search.paths += /vendor/${LIB}\n"
  "namespace.default.asan.permitted.paths = /data:/vendor\n"
  "namespace.default.links = system\n"
  "namespace.default.links += vndk\n"
  // irregular whitespaces are added intentionally for testing purpose
  "namespace.default.link.system.shared_libs=  libc.so\n"
  "namespace.default.link.system.shared_libs +=   libm.so:libdl.so\n"
  "namespace.default.link.system.shared_libs   +=libstdc++.so\n"
  "namespace.default.link.vndk.shared_libs = libcutils.so:libbase.so\n"
  "namespace.system.isolated = true\n"
  "namespace.system.visible = true\n"
  "namespace.system.search.paths = /system/${LIB}\n"
  "namespace.system.permitted.paths = /system/${LIB}\n"
  "namespace.system.asan.search.paths = /data:/system/${LIB}\n"
  "namespace.system.asan.permitted.paths = /data:/system\n"
  "namespace.vndk.isolated = tr\n"
  "namespace.vndk.isolated += ue\n" // should be ignored and return as 'false'.
  "namespace.vndk.search.paths = /system/${LIB}/vndk\n"
  "namespace.vndk.asan.search.paths = /data\n"
  "namespace.vndk.asan.search.paths += /system/${LIB}/vndk\n"
  "namespace.vndk.links = default\n"
  "namespace.vndk.link.default.allow_all_shared_libs = true\n"
  "namespace.vndk.link.vndk_in_system.allow_all_shared_libs = true\n"
  "namespace.vndk_in_system.isolated = true\n"
  "namespace.vndk_in_system.visible = true\n"
  "namespace.vndk_in_system.search.paths = /system/${LIB}\n"
  "namespace.vndk_in_system.permitted.paths = /system/${LIB}\n"
  "namespace.vndk_in_system.whitelisted = libz.so:libyuv.so:libtinyxml2.so\n"
  "\n";

static bool write_version(const std::string& path, uint32_t version) {
  std::string content = android::base::StringPrintf("%d", version);
  return android::base::WriteStringToFile(content, path);
}

static std::vector<std::string> resolve_paths(std::vector<std::string> paths) {
  std::vector<std::string> resolved_paths;
  resolve_paths(paths, &resolved_paths);
  return resolved_paths;
}

static void run_linker_config_smoke_test(bool is_asan) {
  const std::vector<std::string> kExpectedDefaultSearchPath =
      resolve_paths(is_asan ? std::vector<std::string>({ "/data", "/vendor/lib" ARCH_SUFFIX }) :
                              std::vector<std::string>({ "/vendor/lib" ARCH_SUFFIX }));

  const std::vector<std::string> kExpectedDefaultPermittedPath =
      resolve_paths(is_asan ? std::vector<std::string>({ "/data", "/vendor" }) :
                              std::vector<std::string>({ "/vendor/lib" ARCH_SUFFIX }));

  const std::vector<std::string> kExpectedSystemSearchPath =
      resolve_paths(is_asan ? std::vector<std::string>({ "/data", "/system/lib" ARCH_SUFFIX }) :
                              std::vector<std::string>({ "/system/lib" ARCH_SUFFIX }));

  const std::vector<std::string> kExpectedSystemPermittedPath =
      resolve_paths(is_asan ? std::vector<std::string>({ "/data", "/system" }) :
                              std::vector<std::string>({ "/system/lib" ARCH_SUFFIX }));

  const std::vector<std::string> kExpectedVndkSearchPath =
      resolve_paths(is_asan ? std::vector<std::string>({ "/data", "/system/lib" ARCH_SUFFIX "/vndk"}) :
                              std::vector<std::string>({ "/system/lib" ARCH_SUFFIX "/vndk"}));

  TemporaryFile tmp_file;
  close(tmp_file.fd);
  tmp_file.fd = -1;

  android::base::WriteStringToFile(config_str, tmp_file.path);

  TemporaryDir tmp_dir;

  std::string executable_path = std::string(tmp_dir.path) + "/some-binary";
  std::string version_file = std::string(tmp_dir.path) + "/.version";

  auto file_guard =
      android::base::make_scope_guard([&version_file] { unlink(version_file.c_str()); });

  ASSERT_TRUE(write_version(version_file, 113U)) << strerror(errno);

  // read config
  const Config* config = nullptr;
  std::string error_msg;
  ASSERT_TRUE(Config::read_binary_config(tmp_file.path,
                                         executable_path.c_str(),
                                         is_asan,
                                         &config,
                                         &error_msg)) << error_msg;
  ASSERT_TRUE(config != nullptr);
  ASSERT_TRUE(error_msg.empty());

  ASSERT_EQ(113, config->target_sdk_version());

  const NamespaceConfig* default_ns_config = config->default_namespace_config();
  ASSERT_TRUE(default_ns_config != nullptr);

  ASSERT_TRUE(default_ns_config->isolated());
  ASSERT_FALSE(default_ns_config->visible());
  ASSERT_EQ(kExpectedDefaultSearchPath, default_ns_config->search_paths());
  ASSERT_EQ(kExpectedDefaultPermittedPath, default_ns_config->permitted_paths());

  const auto& default_ns_links = default_ns_config->links();
  ASSERT_EQ(2U, default_ns_links.size());

  ASSERT_EQ("system", default_ns_links[0].ns_name());
  ASSERT_EQ("libc.so:libm.so:libdl.so:libstdc++.so", default_ns_links[0].shared_libs());
  ASSERT_FALSE(default_ns_links[0].allow_all_shared_libs());

  ASSERT_EQ("vndk", default_ns_links[1].ns_name());
  ASSERT_EQ("libcutils.so:libbase.so", default_ns_links[1].shared_libs());
  ASSERT_FALSE(default_ns_links[1].allow_all_shared_libs());

  auto& ns_configs = config->namespace_configs();
  ASSERT_EQ(4U, ns_configs.size());

  // find second namespace
  const NamespaceConfig* ns_system = nullptr;
  const NamespaceConfig* ns_vndk = nullptr;
  const NamespaceConfig* ns_vndk_in_system = nullptr;
  for (auto& ns : ns_configs) {
    std::string ns_name = ns->name();
    ASSERT_TRUE(ns_name == "system" || ns_name == "default" ||
                ns_name == "vndk" || ns_name == "vndk_in_system")
        << "unexpected ns name: " << ns->name();

    if (ns_name == "system") {
      ns_system = ns.get();
    } else if (ns_name == "vndk") {
      ns_vndk = ns.get();
    } else if (ns_name == "vndk_in_system") {
      ns_vndk_in_system = ns.get();
    }
  }

  ASSERT_TRUE(ns_system != nullptr) << "system namespace was not found";

  ASSERT_TRUE(ns_system->isolated());
  ASSERT_TRUE(ns_system->visible());
  ASSERT_EQ(kExpectedSystemSearchPath, ns_system->search_paths());
  ASSERT_EQ(kExpectedSystemPermittedPath, ns_system->permitted_paths());

  ASSERT_TRUE(ns_vndk != nullptr) << "vndk namespace was not found";

  ASSERT_FALSE(ns_vndk->isolated()); // malformed bool property
  ASSERT_FALSE(ns_vndk->visible()); // undefined bool property
  ASSERT_EQ(kExpectedVndkSearchPath, ns_vndk->search_paths());

  const auto& ns_vndk_links = ns_vndk->links();
  ASSERT_EQ(1U, ns_vndk_links.size());
  ASSERT_EQ("default", ns_vndk_links[0].ns_name());
  ASSERT_TRUE(ns_vndk_links[0].allow_all_shared_libs());

  ASSERT_TRUE(ns_vndk_in_system != nullptr) << "vndk_in_system namespace was not found";
  ASSERT_EQ(
      std::vector<std::string>({"libz.so", "libyuv.so", "libtinyxml2.so"}),
      ns_vndk_in_system->whitelisted_libs());
}

TEST(linker_config, smoke) {
  run_linker_config_smoke_test(false);
}

TEST(linker_config, asan_smoke) {
  run_linker_config_smoke_test(true);
}

TEST(linker_config, ns_link_shared_libs_invalid_settings) {
  // This unit test ensures an error is emitted when a namespace link in ld.config.txt specifies
  // both shared_libs and allow_all_shared_libs.

  static const char config_str[] =
    "dir.test = /data/local/tmp\n"
    "\n"
    "[test]\n"
    "additional.namespaces = system\n"
    "namespace.default.links = system\n"
    "namespace.default.link.system.shared_libs = libc.so:libm.so\n"
    "namespace.default.link.system.allow_all_shared_libs = true\n"
    "\n";

  TemporaryFile tmp_file;
  close(tmp_file.fd);
  tmp_file.fd = -1;

  android::base::WriteStringToFile(config_str, tmp_file.path);

  TemporaryDir tmp_dir;

  std::string executable_path = std::string(tmp_dir.path) + "/some-binary";

  const Config* config = nullptr;
  std::string error_msg;
  ASSERT_FALSE(Config::read_binary_config(tmp_file.path,
                                          executable_path.c_str(),
                                          false,
                                          &config,
                                          &error_msg));
  ASSERT_TRUE(config == nullptr);
  ASSERT_EQ(std::string(tmp_file.path) + ":6: "
            "error: both shared_libs and allow_all_shared_libs are set for default->system link.",
            error_msg);
}

TEST(linker_config, dir_path_resolve) {
  // This unit test ensures the linker resolves paths of dir.${section}
  // properties to real path.

  TemporaryDir tmp_dir;

  std::string sub_dir = std::string(tmp_dir.path) + "/subdir";
  mkdir(sub_dir.c_str(), 0755);

  auto subdir_guard =
      android::base::make_scope_guard([&sub_dir] { rmdir(sub_dir.c_str()); });

  std::string symlink_path = std::string(tmp_dir.path) + "/symlink";
  symlink(sub_dir.c_str(), symlink_path.c_str());

  auto symlink_guard =
      android::base::make_scope_guard([&symlink_path] { unlink(symlink_path.c_str()); });

  std::string config_str =
      "dir.test = " + symlink_path + "\n"
      "\n"
      "[test]\n";

  TemporaryFile tmp_file;
  close(tmp_file.fd);
  tmp_file.fd = -1;

  android::base::WriteStringToFile(config_str, tmp_file.path);

  std::string executable_path = sub_dir + "/some-binary";

  const Config* config = nullptr;
  std::string error_msg;

  ASSERT_TRUE(Config::read_binary_config(tmp_file.path,
                                         executable_path.c_str(),
                                         false,
                                         &config,
                                         &error_msg)) << error_msg;

  ASSERT_TRUE(config != nullptr) << error_msg;
  ASSERT_TRUE(error_msg.empty()) << error_msg;
}
