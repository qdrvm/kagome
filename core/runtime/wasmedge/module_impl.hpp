/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_WASMEDGE_MODULE_IMPL
#define KAGOME_CORE_RUNTIME_WASMEDGE_MODULE_IMPL

#include "runtime/module.hpp"

#include "common/buffer.hpp"
#include "runtime/trie_storage_provider.hpp"

namespace kagome::storage::trie {
  class EphemeralTrieBatch;
}

struct WasmEdge_ASTModuleContext;

namespace kagome::runtime::wasmedge {

  class InstanceEnvironmentFactory;

  /**
   * Stores a wasm::Module and a wasm::Module instance which contains the module
   * and the provided runtime external interface
   */
  class ModuleImpl final : public runtime::Module,
                           public std::enable_shared_from_this<ModuleImpl> {
   public:
    static constexpr auto kDefaultHeappages = 1024;
    enum class Error { EMPTY_STATE_CODE = 1, INVALID_STATE_CODE };

    ModuleImpl(ModuleImpl &&) = default;
    ModuleImpl &operator=(ModuleImpl &&) = default;

    ModuleImpl(const ModuleImpl &) = delete;
    ModuleImpl &operator=(const ModuleImpl &) = delete;

    ~ModuleImpl() override = default;

    static outcome::result<std::unique_ptr<ModuleImpl>> createFromCode(
        const std::vector<uint8_t> &code,
        std::shared_ptr<const InstanceEnvironmentFactory> env_factory_);

    outcome::result<std::shared_ptr<ModuleInstance>> instantiate()
        const override;

    const WasmEdge_ASTModuleContext* ast() const {
      return ast_ctx_;
    }

   private:
    ModuleImpl(const WasmEdge_ASTModuleContext *ast_ctx,
               std::shared_ptr<const InstanceEnvironmentFactory> env_factory);

    std::shared_ptr<const InstanceEnvironmentFactory> env_factory_;
    const WasmEdge_ASTModuleContext *ast_ctx_;
  };

}  // namespace kagome::runtime::wasmedge

#endif  // KAGOME_CORE_RUNTIME_WASMEDGE_MODULE_IMPL
