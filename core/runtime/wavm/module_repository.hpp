/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_WAVM_IMPL_MODULE_REPOSITORY_HPP
#define KAGOME_CORE_RUNTIME_WAVM_IMPL_MODULE_REPOSITORY_HPP

#include <memory>
#include <unordered_map>

#include <gsl/span>

#include "log/logger.hpp"
#include "outcome/outcome.hpp"
#include "runtime/runtime_code_provider.hpp"
#include "runtime/wavm/impl/module.hpp"
#include "runtime/wavm/impl/module_instance.hpp"
#include "storage/trie/types.hpp"

namespace WAVM::Runtime {
  class Compartment;
}

namespace kagome::runtime::wavm {

  class ModuleRepository final {
   public:
    ModuleRepository(
        std::shared_ptr<IntrinsicResolver>,
        std::shared_ptr<RuntimeCodeProvider>);

    outcome::result<std::shared_ptr<ModuleInstance>> getInstanceAt(
        const storage::trie::RootHash &state);

    outcome::result<std::unique_ptr<Module>> loadFrom(
        gsl::span<const uint8_t> byte_code);

   private:
    std::unordered_map<storage::trie::RootHash, std::shared_ptr<Module>>
        modules_;
    std::unordered_map<storage::trie::RootHash, std::shared_ptr<ModuleInstance>>
        instances_;
    // TODO(Harrm) as it's not a GCPointer, might want to cleanup it in destructor
    WAVM::Runtime::Compartment* compartment_;
    std::shared_ptr<RuntimeCodeProvider> code_provider_;
    std::shared_ptr<IntrinsicResolver> resolver_;
    log::Logger logger_;
  };

}  // namespace kagome::runtime::wavm

#endif  // KAGOME_CORE_RUNTIME_WAVM_IMPL_MODULE_REPOSITORY_HPP
