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
#include "runtime/types.hpp"
#include "runtime/wasm_result.hpp"

namespace kagome::runtime {
  class WasmMemory;
  class Core;
  class WasmProvider;

  namespace binaryen {
    class RuntimeEnvironmentFactory;
    class CoreFactory;
  }  // namespace binaryen
}  // namespace kagome::runtime

namespace kagome::host_api {
  /**
   * Implements miscellaneous extension functions
   */
  class MiscExtension final {
   public:
    MiscExtension(
        uint64_t chain_id,
        std::shared_ptr<runtime::binaryen::CoreFactory> core_api_factory,
        std::shared_ptr<runtime::binaryen::RuntimeEnvironmentFactory>
            runtime_env_factory,
        std::shared_ptr<runtime::WasmMemory> memory);

    ~MiscExtension() = default;

    /**
     * @return id (a 64-bit unsigned integer) of the current chain
     */
    uint64_t ext_chain_id() const;

    runtime::WasmResult ext_misc_runtime_version_version_1(
        runtime::WasmSpan data) const;

    void ext_misc_print_hex_version_1(runtime::WasmSpan data) const;

    void ext_misc_print_num_version_1(uint64_t value) const;

    void ext_misc_print_utf8_version_1(runtime::WasmSpan data) const;

   private:
    std::shared_ptr<runtime::binaryen::CoreFactory> core_api_factory_;
    std::shared_ptr<runtime::binaryen::RuntimeEnvironmentFactory>
        runtime_env_factory_;
    std::shared_ptr<runtime::WasmMemory> memory_;
    log::Logger logger_;
    const uint64_t chain_id_ = 42;
  };
}  // namespace kagome::host_api

#endif  // KAGOME_MISC_EXTENSION_HPP
