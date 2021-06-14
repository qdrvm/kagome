/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_WAVM_CORE_API_PROVIDER_HPP
#define KAGOME_CORE_RUNTIME_WAVM_CORE_API_PROVIDER_HPP

#include "../../../../deps/wavm/Lib/Runtime/RuntimePrivate.h"
#include "runtime/core_api_provider.hpp"

#include <memory>

namespace kagome::storage::changes_trie {
  class ChangesTracker;
}

namespace kagome::blockchain {
  class BlockHeaderRepository;
}

namespace kagome::runtime {
  class TrieStorageProvider;
  class Memory;
}  // namespace kagome::runtime

namespace kagome::runtime::wavm {

  class ModuleRepository;
  class IntrinsicModuleInstance;

  class CoreApiProvider final
      : public runtime::CoreApiProvider,
        public std::enable_shared_from_this<CoreApiProvider> {
   public:
    CoreApiProvider(
        WAVM::Runtime::Compartment* compartment,
        std::shared_ptr<runtime::wavm::IntrinsicModuleInstance> intrinsic_module,
        std::shared_ptr<runtime::TrieStorageProvider> storage_provider,
        std::shared_ptr<blockchain::BlockHeaderRepository> block_header_repo,
        std::shared_ptr<storage::changes_trie::ChangesTracker> changes_tracker,
        std::shared_ptr<host_api::HostApiFactory> host_api_factory);

    std::unique_ptr<Core> makeCoreApi(
        std::shared_ptr<const crypto::Hasher> hasher,
        gsl::span<uint8_t> runtime_code) const override;

   private:
    WAVM::Runtime::Compartment* compartment_;
    std::shared_ptr<runtime::wavm::IntrinsicModuleInstance> intrinsic_module_;
    std::shared_ptr<runtime::TrieStorageProvider> storage_provider_;
    std::shared_ptr<blockchain::BlockHeaderRepository> block_header_repo_;
    std::shared_ptr<storage::changes_trie::ChangesTracker> changes_tracker_;
    std::shared_ptr<host_api::HostApiFactory> host_api_factory_;
  };

}  // namespace kagome::runtime::wavm

#endif  // KAGOME_CORE_RUNTIME_WAVM_CORE_API_PROVIDER_HPP
