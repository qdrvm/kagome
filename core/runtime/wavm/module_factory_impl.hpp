/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "runtime/module_factory.hpp"

#include "outcome/outcome.hpp"
#include "runtime/wabt/instrument.hpp"

namespace kagome::application {
  class AppConfiguration;
}

namespace kagome::crypto {
  class Hasher;
}

namespace kagome::host_api {
  class HostApiFactory;
}

namespace kagome::runtime {
  class CoreApiFactory;
}  // namespace kagome::runtime

namespace kagome::storage::trie {
  class TrieStorage;
  class TrieSerializer;
}  // namespace kagome::storage::trie

namespace kagome::runtime {
  class WasmInstrumenter;
}

namespace kagome::runtime::wavm {

  class CompartmentWrapper;

  class IntrinsicModule;
  struct ModuleParams;

  class ModuleFactoryImpl final : public ModuleFactory {
   public:
    ModuleFactoryImpl(
        std::shared_ptr<CompartmentWrapper> compartment,
        std::shared_ptr<ModuleParams> module_params,
        std::shared_ptr<host_api::HostApiFactory> host_api_factory,
        std::shared_ptr<CoreApiFactory> core_factory,
        std::shared_ptr<storage::trie::TrieStorage> storage,
        std::shared_ptr<storage::trie::TrieSerializer> serializer,
        std::shared_ptr<IntrinsicModule> intrinsic_module,
        std::shared_ptr<crypto::Hasher> hasher);

    std::optional<std::string> compilerType() const override;
    CompilationOutcome<void> compile(std::filesystem::path path_compiled,
                                     BufferView code) const override;
    CompilationOutcome<std::shared_ptr<Module>> loadCompiled(
        std::filesystem::path path_compiled) const override;

   private:
    std::shared_ptr<CompartmentWrapper> compartment_;
    std::shared_ptr<ModuleParams> module_params_;
    std::shared_ptr<host_api::HostApiFactory> host_api_factory_;
    std::shared_ptr<CoreApiFactory> core_factory_;
    std::shared_ptr<storage::trie::TrieStorage> storage_;
    std::shared_ptr<storage::trie::TrieSerializer> serializer_;
    std::shared_ptr<IntrinsicModule> intrinsic_module_;
    std::shared_ptr<crypto::Hasher> hasher_;
    std::shared_ptr<const WasmInstrumenter> instrumenter_;
  };

}  // namespace kagome::runtime::wavm
