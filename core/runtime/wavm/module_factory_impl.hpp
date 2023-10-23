/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "runtime/module_factory.hpp"

#include "outcome/outcome.hpp"

namespace kagome::application {
  class AppConfiguration;
}

namespace kagome::crypto {
  class Hasher;
}

namespace kagome::host_api {
  class HostApiFactory;
}

namespace kagome::storage::trie {
  class TrieStorage;
  class TrieSerializer;
}  // namespace kagome::storage::trie

namespace kagome::runtime {
  class SingleModuleCache;
}

namespace kagome::runtime::wavm {

  class CompartmentWrapper;

  class IntrinsicModule;
  struct ModuleParams;
  struct ModuleCache;

  class ModuleFactoryImpl final
      : public ModuleFactory,
        public std::enable_shared_from_this<ModuleFactoryImpl> {
   public:
    ModuleFactoryImpl(
        std::shared_ptr<CompartmentWrapper> compartment,
        std::shared_ptr<ModuleParams> module_params,
        std::shared_ptr<host_api::HostApiFactory> host_api_factory,
        std::shared_ptr<storage::trie::TrieStorage> storage,
        std::shared_ptr<storage::trie::TrieSerializer> serializer,
        std::shared_ptr<IntrinsicModule> intrinsic_module,
        std::shared_ptr<SingleModuleCache> last_compiled_module,
        std::optional<std::shared_ptr<ModuleCache>> module_cache,
        std::shared_ptr<crypto::Hasher> hasher);

    outcome::result<std::shared_ptr<Module>> make(
        gsl::span<const uint8_t> code) const override;

   private:
    std::shared_ptr<CompartmentWrapper> compartment_;
    std::shared_ptr<ModuleParams> module_params_;
    std::shared_ptr<host_api::HostApiFactory> host_api_factory_;
    std::shared_ptr<storage::trie::TrieStorage> storage_;
    std::shared_ptr<storage::trie::TrieSerializer> serializer_;
    std::shared_ptr<SingleModuleCache> last_compiled_module_;
    std::shared_ptr<IntrinsicModule> intrinsic_module_;
    std::shared_ptr<crypto::Hasher> hasher_;
  };

}  // namespace kagome::runtime::wavm
