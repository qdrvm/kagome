/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "runtime/instance_environment.hpp"

namespace kagome::storage::trie {
  class TrieStorage;
  class TrieSerializer;
}  // namespace kagome::storage::trie

namespace kagome::host_api {
  class HostApiFactory;
}

namespace kagome::runtime {
  class CoreApiFactory;
}

namespace kagome::runtime::binaryen {

  class RuntimeExternalInterface;

  struct BinaryenInstanceEnvironment {
    InstanceEnvironment env;
    std::shared_ptr<RuntimeExternalInterface> rei;
  };

  class InstanceEnvironmentFactory final {
   public:
    InstanceEnvironmentFactory(
        std::shared_ptr<storage::trie::TrieStorage> storage,
        std::shared_ptr<storage::trie::TrieSerializer> serializer,
        std::shared_ptr<CoreApiFactory> core_factory,
        std::shared_ptr<host_api::HostApiFactory> host_api_factory);

    [[nodiscard]] BinaryenInstanceEnvironment make() const;

   private:
    std::shared_ptr<storage::trie::TrieStorage> storage_;
    std::shared_ptr<storage::trie::TrieSerializer> serializer_;
    std::shared_ptr<CoreApiFactory> core_factory_;
    std::shared_ptr<host_api::HostApiFactory> host_api_factory_;
    std::shared_ptr<const WasmInstrumenter> instrumenter_;
  };

}  // namespace kagome::runtime::binaryen
