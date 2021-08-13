/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_BINARYEN_INSTANCE_ENVIRONMENT_FACTORY_HPP
#define KAGOME_CORE_RUNTIME_BINARYEN_INSTANCE_ENVIRONMENT_FACTORY_HPP

#include "runtime/instance_environment.hpp"

namespace kagome::storage::trie {
  class TrieStorage;
}

namespace kagome::host_api {
  class HostApiFactory;
}

namespace kagome::blockchain {
  class BlockHeaderRepository;
}

namespace kagome::runtime::binaryen {

  class RuntimeExternalInterface;

  struct BinaryenInstanceEnvironment {
    InstanceEnvironment env;
    std::shared_ptr<RuntimeExternalInterface> rei;
  };

  class InstanceEnvironmentFactory final
      : public std::enable_shared_from_this<InstanceEnvironmentFactory> {
   public:
    InstanceEnvironmentFactory(
        std::shared_ptr<storage::trie::TrieStorage> storage,
        std::shared_ptr<host_api::HostApiFactory> host_api_factory,
        std::shared_ptr<blockchain::BlockHeaderRepository> block_header_repo);

    [[nodiscard]] BinaryenInstanceEnvironment make() const;

   private:
    std::shared_ptr<storage::trie::TrieStorage> storage_;
    std::shared_ptr<host_api::HostApiFactory> host_api_factory_;
    std::shared_ptr<blockchain::BlockHeaderRepository> block_header_repo_;
  };

}  // namespace kagome::runtime::binaryen

#endif  // KAGOME_CORE_RUNTIME_BINARYEN_INSTANCE_ENVIRONMENT_FACTORY_HPP
