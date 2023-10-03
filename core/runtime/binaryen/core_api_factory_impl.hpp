/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_BINARYEN_CORE_API_FACTORY_IMPL_HPP
#define KAGOME_CORE_RUNTIME_BINARYEN_CORE_API_FACTORY_IMPL_HPP

#include "runtime/core_api_factory.hpp"
#include "runtime/runtime_api/core.hpp"

namespace kagome::storage::trie {
  class TrieStorage;
}

namespace kagome::blockchain {
  class BlockHeaderRepository;
}

namespace kagome::runtime {
  class TrieStorageProvider;
  class Memory;
  class RuntimeEnvironmentFactory;
  class RuntimePropertiesCache;
  class ModuleFactory;
}  // namespace kagome::runtime

namespace kagome::runtime::binaryen {

  class CoreApiFactoryImpl final
      : public runtime::CoreApiFactory,
        public std::enable_shared_from_this<CoreApiFactoryImpl> {
   public:
    CoreApiFactoryImpl(std::shared_ptr<const ModuleFactory> module_factory);

    outcome::result<std::unique_ptr<RestrictedCore>> make(
        std::shared_ptr<const crypto::Hasher> hasher,
        const std::vector<uint8_t> &runtime_code) const override;

   private:
    std::shared_ptr<const ModuleFactory> module_factory_;
  };

}  // namespace kagome::runtime::binaryen

#endif  // KAGOME_CORE_RUNTIME_BINARYEN_CORE_API_FACTORY_IMPL_HPP
