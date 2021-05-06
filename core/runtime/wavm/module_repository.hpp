/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_WAVM_IMPL_MODULE_REPOSITORY_HPP
#define KAGOME_CORE_RUNTIME_WAVM_IMPL_MODULE_REPOSITORY_HPP

#include <memory>
#include <unordered_map>

#include <gsl/span>

#include "crypto/hasher.hpp"
#include "log/logger.hpp"
#include "outcome/outcome.hpp"
#include "runtime/runtime_code_provider.hpp"
#include "runtime/wavm/impl/memory.hpp"
#include "runtime/wavm/impl/module.hpp"
#include "runtime/wavm/impl/module_instance.hpp"
#include "storage/trie/types.hpp"
#include "host_api/host_api.hpp"

namespace WAVM::Runtime {
  class Compartment;
}

namespace kagome::runtime::wavm {

  class Memory;

  class ModuleRepository final {
   public:
    ModuleRepository(std::shared_ptr<crypto::Hasher> hasher,
                     std::shared_ptr<Memory>,
                     std::shared_ptr<IntrinsicResolver>,
                     std::shared_ptr<RuntimeCodeProvider>);

    outcome::result<std::shared_ptr<ModuleInstance>> getInstanceAt(
        const storage::trie::RootHash &state);

    outcome::result<std::unique_ptr<Module>> loadFrom(
        gsl::span<const uint8_t> byte_code);

   private:
    std::unordered_map<common::Hash256, std::shared_ptr<Module>> modules_;
    std::unordered_map<common::Hash256, std::shared_ptr<ModuleInstance>>
        instances_;
    // TODO(Harrm) as it's not a GCPointer, might want to cleanup it in
    // destructor
    WAVM::Runtime::Compartment *compartment_;
    std::shared_ptr<RuntimeCodeProvider> code_provider_;
    std::shared_ptr<IntrinsicResolver> resolver_;
    std::shared_ptr<Memory> memory_;
    std::shared_ptr<crypto::Hasher> hasher_;
    log::Logger logger_;
  };

}  // namespace kagome::runtime::wavm

#endif  // KAGOME_CORE_RUNTIME_WAVM_IMPL_MODULE_REPOSITORY_HPP
