/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "runtime/core_api_factory.hpp"

#include <memory>

namespace kagome::storage::trie {
  class TrieStorage;
}

namespace kagome::blockchain {
  class BlockHeaderRepository;
}

namespace kagome::runtime {
  class Memory;
  class ModuleRepository;
  class SingleModuleCache;
  class RuntimePropertiesCache;
}  // namespace kagome::runtime

namespace kagome::runtime::wavm {

  class IntrinsicModule;
  class CompartmentWrapper;
  class InstanceEnvironmentFactory;
  struct ModuleParams;

  class CoreApiFactoryImpl final
      : public runtime::CoreApiFactory,
        public std::enable_shared_from_this<CoreApiFactoryImpl> {
   public:
    CoreApiFactoryImpl(
        std::shared_ptr<CompartmentWrapper> compartment,
        std::shared_ptr<ModuleParams> module_params,
        std::shared_ptr<IntrinsicModule> intrinsic_module,
        std::shared_ptr<storage::trie::TrieStorage> storage,
        std::shared_ptr<blockchain::BlockHeaderRepository> block_header_repo,
        std::shared_ptr<const InstanceEnvironmentFactory> instance_env_factory,
        std::shared_ptr<SingleModuleCache> last_compiled_module,
        std::shared_ptr<runtime::RuntimePropertiesCache> cache);

    [[nodiscard]] std::unique_ptr<Core> make(
        std::shared_ptr<const crypto::Hasher> hasher,
        const std::vector<uint8_t> &runtime_code) const override;

   private:
    std::shared_ptr<const InstanceEnvironmentFactory> instance_env_factory_;
    std::shared_ptr<CompartmentWrapper> compartment_;
    std::shared_ptr<ModuleParams> module_params_;
    std::shared_ptr<IntrinsicModule> intrinsic_module_;
    std::shared_ptr<storage::trie::TrieStorage> storage_;
    std::shared_ptr<blockchain::BlockHeaderRepository> block_header_repo_;
    std::shared_ptr<SingleModuleCache> last_compiled_module_;
    std::shared_ptr<runtime::RuntimePropertiesCache> cache_;
  };

}  // namespace kagome::runtime::wavm
