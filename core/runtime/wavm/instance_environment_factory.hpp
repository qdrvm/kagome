/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_WAVM_INSTANCE_ENVIRONMENT_FACTORY_HPP
#define KAGOME_CORE_RUNTIME_WAVM_INSTANCE_ENVIRONMENT_FACTORY_HPP

#include "runtime/instance_environment.hpp"

namespace kagome::host_api {
  class HostApiFactory;
}

namespace kagome::storage::trie {
  class TrieStorage;
}

namespace kagome::blockchain {
  class BlockHeaderRepository;
}

namespace kagome::runtime::wavm {

  class IntrinsicModule;
  class IntrinsicResolver;
  class CompartmentWrapper;

  struct WavmInstanceEnvironment {
    InstanceEnvironment env;
    std::shared_ptr<IntrinsicResolver> resolver;
  };

  class InstanceEnvironmentFactory final
      : public std::enable_shared_from_this<InstanceEnvironmentFactory> {
   public:
    InstanceEnvironmentFactory(
        std::shared_ptr<storage::trie::TrieStorage> storage,
        std::shared_ptr<CompartmentWrapper> compartment,
        std::shared_ptr<runtime::wavm::IntrinsicModule> intrinsic_module,
        std::shared_ptr<host_api::HostApiFactory> host_api_factory,
        std::shared_ptr<blockchain::BlockHeaderRepository> block_header_repo);

    [[nodiscard]] WavmInstanceEnvironment make() const;

   private:
    std::shared_ptr<storage::trie::TrieStorage> storage_;
    std::shared_ptr<CompartmentWrapper> compartment_;
    std::shared_ptr<runtime::wavm::IntrinsicModule> intrinsic_module_;
    std::shared_ptr<host_api::HostApiFactory> host_api_factory_;
    std::shared_ptr<blockchain::BlockHeaderRepository> block_header_repo_;
  };

}  // namespace kagome::runtime::wavm

#endif  // KAGOME_CORE_RUNTIME_WAVM_INSTANCE_ENVIRONMENT_FACTORY_HPP
