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

namespace kagome::blockchain {
  class BlockHeaderRepository;
}

namespace kagome::runtime {
  class TrieStorageProvider;

  namespace wavm {
    class ModuleRepository;
    class Memory;
    class IntrinsicResolver;
  }

}  // namespace kagome::runtime

namespace kagome::host_api {
  /**
   * Implements miscellaneous extension functions
   */
  class MiscExtension final {
   public:
    MiscExtension(
        uint64_t chain_id,
        std::shared_ptr<runtime::wavm::ModuleRepository> module_repo,
        std::shared_ptr<runtime::wavm::Memory> memory,
        std::shared_ptr<runtime::wavm::IntrinsicResolver> intrinsic_resolver,
        std::shared_ptr<runtime::TrieStorageProvider> storage_provider,
        std::shared_ptr<blockchain::BlockHeaderRepository> block_header_repo);

    ~MiscExtension() = default;

    runtime::WasmSpan ext_misc_runtime_version_version_1(
        runtime::WasmSpan data) const;

    void ext_misc_print_hex_version_1(runtime::WasmSpan data) const;

    void ext_misc_print_num_version_1(uint64_t value) const;

    void ext_misc_print_utf8_version_1(runtime::WasmSpan data) const;

   private:
    std::shared_ptr<runtime::wavm::ModuleRepository> module_repo_;
    std::shared_ptr<runtime::wavm::Memory> memory_;
    std::shared_ptr<runtime::wavm::IntrinsicResolver> intrinsic_resolver_;
    std::shared_ptr<runtime::TrieStorageProvider> storage_provider_;
    std::shared_ptr<blockchain::BlockHeaderRepository> block_header_repo_;
    log::Logger logger_;
    const uint64_t chain_id_ = 42;
  };
}  // namespace kagome::host_api

#endif  // KAGOME_MISC_EXTENSION_HPP
