/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_BINARYEN_RUNTIME_API_CORE_FACTORY_IMPL
#define KAGOME_CORE_RUNTIME_BINARYEN_RUNTIME_API_CORE_FACTORY_IMPL

#include "runtime/binaryen/runtime_manager.hpp"
#include "runtime/core_factory.hpp"

namespace kagome::blockchain {
  class BlockHeaderRepository;
}
namespace kagome::storage::changes_trie {
  class ChangesTracker;
}

namespace kagome::runtime::binaryen {

  class CoreFactoryImpl : public CoreFactory {
   public:
    CoreFactoryImpl(
        std::shared_ptr<RuntimeManager> runtime_manager,
        std::shared_ptr<storage::changes_trie::ChangesTracker> changes_tracker,
        std::shared_ptr<blockchain::BlockHeaderRepository> header_repo);
    ~CoreFactoryImpl() override = default;

    std::unique_ptr<Core> createWithCode(
        std::shared_ptr<WasmProvider> wasm_provider) override;

   private:
    std::shared_ptr<RuntimeManager> runtime_manager_;
    std::shared_ptr<storage::changes_trie::ChangesTracker> changes_tracker_;
    std::shared_ptr<blockchain::BlockHeaderRepository> header_repo_;
  };

}  // namespace kagome::runtime::binaryen

#endif  // KAGOME_CORE_RUNTIME_BINARYEN_RUNTIME_API_CORE_FACTORY_IMPL
