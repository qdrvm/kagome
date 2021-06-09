/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_MISC_EXTENSION_HPP
#define KAGOME_MISC_EXTENSION_HPP

#include <cstdint>
#include <functional>
#include <memory>

#include "log/logger.hpp"
#include "outcome/outcome.hpp"
#include "runtime/ptr_size.hpp"
#include "runtime/types.hpp"

namespace kagome::runtime {
  class CoreApiProvider;
  class MemoryProvider;
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
    MiscExtension(uint64_t chain_id,
                  std::shared_ptr<const crypto::Hasher> hasher,
                  std::shared_ptr<const runtime::MemoryProvider> memory_provider,
                  std::shared_ptr<const runtime::CoreApiProvider> core_provider);

    ~MiscExtension() = default;

    runtime::WasmSpan ext_misc_runtime_version_version_1(
        runtime::WasmSpan data) const;

    void ext_misc_print_hex_version_1(runtime::WasmSpan data) const;

    void ext_misc_print_num_version_1(uint64_t value) const;

    void ext_misc_print_utf8_version_1(runtime::WasmSpan data) const;

   private:
    std::shared_ptr<const crypto::Hasher> hasher_;
    std::shared_ptr<const runtime::MemoryProvider> memory_provider_;
    std::shared_ptr<const runtime::CoreApiProvider> core_provider_;
    log::Logger logger_;
    const uint64_t chain_id_ = 42;
  };

}  // namespace kagome::host_api

#endif  // KAGOME_MISC_EXTENSION_HPP
