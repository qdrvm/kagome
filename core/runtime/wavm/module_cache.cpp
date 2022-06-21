/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/wavm/module_cache.hpp"

#include <fstream>
#include <vector>

#include "crypto/hasher.hpp"
#include "filesystem/directories.hpp"

namespace kagome::runtime::wavm {
  ModuleCache::ModuleCache(const application::AppConfiguration &app_config,
                           std::shared_ptr<crypto::Hasher> hasher)
      : app_config_(app_config),
        hasher_{std::move(hasher)},
        logger_{log::createLogger("WAVM Module Cache", "runtime_cache")} {
    BOOST_ASSERT(hasher_ != nullptr);
  }

  std::vector<WAVM::U8> ModuleCache::getCachedObject(
      const WAVM::U8 *wasmBytes,
      WAVM::Uptr numWASMBytes,
      std::function<std::vector<WAVM::U8>()> &&compileThunk) {
    namespace fs = kagome::filesystem;
    auto runtime_hash =
        hasher_->twox_64(gsl::span(wasmBytes, numWASMBytes)).toHex();
    auto filepath = app_config_.runtimeCachePath(runtime_hash);
    if (!exists(filepath) and !exists(app_config_.runtimeCacheDirPath())
        and !fs::createDirectoryRecursive(app_config_.runtimeCacheDirPath())) {
      SL_ERROR(logger_,
               "Failed to create runtimes cache directory {}",
               app_config_.runtimeCacheDirPath());
    }

    std ::vector<WAVM::U8> module;
    if (std::ifstream file{filepath.c_str(), std::ios::in | std::ios::binary};
        file.is_open()) {
      auto module_size = file_size(filepath);
      module.resize(module_size);
      file.read(reinterpret_cast<char *>(module.data()), module_size);
      if (not file.fail()) {
        SL_VERBOSE(logger_, "WAVM runtime cache hit: {}", filepath);
      } else {
        module.clear();
        SL_ERROR(logger_, "Error reading cached module: {}", filepath);
      }
    }
    if (module.empty()) {
      module = compileThunk();
      if (auto file =
              std::ofstream{filepath.c_str(), std::ios::out | std::ios::binary};
          file.is_open()) {
        file.write(reinterpret_cast<char *>(module.data()), module.size());
        file.close();
        if (not file.fail()) {
          SL_VERBOSE(logger_, "Saved WAVM runtime to cache: {}", filepath);
        } else {
          module.clear();
          SL_ERROR(logger_, "Error writing module to cache: {}", filepath);
        }
      } else {
        SL_ERROR(logger_, "Failed to cache WAVM runtime: {}", filepath);
      }
    }

    return module;
  }
}  // namespace kagome::runtime::wavm
