/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_MISC_EXTENSION_HPP
#define KAGOME_MISC_EXTENSION_HPP

#include <cstdint>
#include <memory>

#include "common/logger.hpp"
#include "runtime/types.hpp"
#include "runtime/wasm_result.hpp"

namespace kagome::runtime {
  class WasmMemory;
  class CoreFactory;
}

namespace kagome::extensions {
  /**
   * Implements miscellaneous extension functions
   */
  class MiscExtension final {
   public:
    MiscExtension(uint64_t chain_id,
                  std::shared_ptr<runtime::WasmMemory> memory,
                  std::shared_ptr<runtime::CoreFactory> core_factory);

    ~MiscExtension() = default;

    /**
     * @return id (a 64-bit unsigned integer) of the current chain
     */
    uint64_t ext_chain_id() const;

    runtime::WasmResult ext_misc_runtime_version_version_1(
        runtime::WasmSpan data) const;

   private:
    std::shared_ptr<runtime::WasmMemory> memory_;
    std::shared_ptr<runtime::CoreFactory> core_factory_;
    common::Logger logger_;
    const uint64_t chain_id_ = 42;
  };
}  // namespace kagome::extensions

#endif  // KAGOME_MISC_EXTENSION_HPP
