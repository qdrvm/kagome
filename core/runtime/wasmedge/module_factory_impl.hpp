/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_WASMEDGE_MODULE_FACTORY_IMPL_HPP
#define KAGOME_CORE_RUNTIME_WASMEDGE_MODULE_FACTORY_IMPL_HPP

#include "runtime/module_factory.hpp"

namespace kagome::runtime {
  class TrieStorageProvider;
}

namespace kagome::host_api {
  class HostApiFactory;
}

namespace kagome::storage::trie {
  class TrieStorage;
}

namespace kagome::blockchain {
  class BlockHeaderRepository;
}

namespace kagome::storage::changes_trie {
  class ChangesTracker;
}

namespace kagome::runtime::wasmedge {

  class InstanceEnvironmentFactory;

  class ModuleFactoryImpl final : public ModuleFactory {
   public:
    ModuleFactoryImpl(std::shared_ptr<InstanceEnvironmentFactory> env_factory,
                      std::shared_ptr<storage::trie::TrieStorage> storage);

    outcome::result<std::unique_ptr<Module>> make(
        storage::trie::RootHash const &state,
        gsl::span<const uint8_t> code) const override;

   private:
    std::shared_ptr<InstanceEnvironmentFactory> env_factory_;
    std::shared_ptr<storage::trie::TrieStorage> storage_;
  };

}  // namespace kagome::runtime::wasmedge

#endif  // KAGOME_CORE_RUNTIME_WASMEDGE_MODULE_FACTORY_IMPL_HPP
