/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_BINARYEN_WASM_MODULE_IMPL
#define KAGOME_CORE_RUNTIME_BINARYEN_WASM_MODULE_IMPL

#include "runtime/module.hpp"

#include "common/buffer.hpp"
#include "runtime/binaryen/module/module_instance_impl.hpp"
#include "runtime/trie_storage_provider.hpp"

namespace wasm {
  using namespace ::wasm;  // NOLINT(google-build-using-namespace)
  class Module;
}  // namespace wasm

namespace kagome::storage::trie {
  class EphemeralTrieBatch;
}

namespace kagome::runtime::binaryen {

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
        std::shared_ptr<const InstanceEnvironmentFactory> env_factory_,
        const common::Hash256 &code_hash);

    outcome::result<std::shared_ptr<ModuleInstance>> instantiate()
        const override;

   private:
    ModuleImpl(std::unique_ptr<wasm::Module> &&module,
               std::shared_ptr<const InstanceEnvironmentFactory> env_factory,
               const common::Hash256 &code_hash);

    std::shared_ptr<const InstanceEnvironmentFactory> env_factory_;
    std::shared_ptr<wasm::Module> module_;  // shared to module instances
    const common::Hash256 code_hash_;
  };

}  // namespace kagome::runtime::binaryen

OUTCOME_HPP_DECLARE_ERROR(kagome::runtime::binaryen, ModuleImpl::Error);

#endif  // KAGOME_CORE_RUNTIME_BINARYEN_WASM_MODULE_IMPL
