/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <cstdint>
#include <functional>
#include <memory>

#include "log/logger.hpp"
#include "outcome/outcome.hpp"
#include "runtime/ptr_size.hpp"
#include "runtime/types.hpp"

namespace kagome::runtime {
  class CoreApiFactory;
  class MemoryProvider;
  class TrieStorageProvider;
}  // namespace kagome::runtime

namespace kagome::crypto {
  class Hasher;
}

namespace kagome::host_api {

  /**
   * Implements miscellaneous extension functions
   */
  class MiscExtension final {
   public:
    MiscExtension(
        uint64_t chain_id,
        std::shared_ptr<const crypto::Hasher> hasher,
        std::shared_ptr<const runtime::MemoryProvider> memory_provider,
        std::shared_ptr<runtime::TrieStorageProvider> storage_provider,
        std::shared_ptr<const runtime::CoreApiFactory> core_provider);

    ~MiscExtension() = default;

    runtime::WasmSpan ext_misc_runtime_version_version_1(
        runtime::WasmSpan data) const;

    void ext_misc_print_hex_version_1(runtime::WasmSpan data) const;

    void ext_misc_print_num_version_1(int64_t value) const;

    void ext_misc_print_utf8_version_1(runtime::WasmSpan data) const;

   private:
    std::shared_ptr<const crypto::Hasher> hasher_;
    std::shared_ptr<const runtime::MemoryProvider> memory_provider_;
    std::shared_ptr<runtime::TrieStorageProvider> storage_provider_;
    std::shared_ptr<const runtime::CoreApiFactory> core_factory_;
    log::Logger logger_;
  };

}  // namespace kagome::host_api
