/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "application/app_configuration.hpp"

#include <WAVM/Runtime/Runtime.h>
#include "filesystem/directories.hpp"
#include "log/logger.hpp"

namespace kagome::crypto {
  class Hasher;
}

namespace kagome::runtime::wavm {
  namespace fs = kagome::filesystem;

  /**
   * WAVM runtime cache. Attempts to fetch precompiled module from fs and saves
   * compiled module upon cache miss.
   *
   */
  struct ModuleCache : public WAVM::Runtime::ObjectCacheInterface {
   public:
    ModuleCache(std::shared_ptr<crypto::Hasher> hasher, fs::path cache_dir);

    std::vector<WAVM::U8> getCachedObject(
        const WAVM::U8 *wasmBytes,
        WAVM::Uptr numWASMBytes,
        std::function<std::vector<WAVM::U8>()> &&compileThunk) override;

   private:
    fs::path cache_dir_;
    std::shared_ptr<crypto::Hasher> hasher_;
    log::Logger logger_;
  };

}  // namespace kagome::runtime::wavm
