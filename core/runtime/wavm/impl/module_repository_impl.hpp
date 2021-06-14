/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_WAVM_IMPL_MODULE_REPOSITORY_IMPL_HPP
#define KAGOME_CORE_RUNTIME_WAVM_IMPL_MODULE_REPOSITORY_IMPL_HPP

#include "runtime/wavm/module_repository.hpp"

#include <memory>
#include <unordered_map>

#include <gsl/span>

#include "crypto/hasher.hpp"
#include "host_api/host_api.hpp"
#include "log/logger.hpp"
#include "outcome/outcome.hpp"
#include "runtime/runtime_code_provider.hpp"
#include "runtime/wavm/impl/memory.hpp"
#include "runtime/wavm/impl/module.hpp"
#include "runtime/wavm/impl/module_instance.hpp"
#include "storage/trie/types.hpp"

namespace WAVM::Runtime {
  struct Compartment;
}

namespace kagome::runtime::wavm {

  class ModuleRepositoryImpl final: public ModuleRepository {
   public:
    ModuleRepositoryImpl(
        WAVM::Runtime::Compartment* compartment,
        std::shared_ptr<const crypto::Hasher> hasher,
                     std::shared_ptr<IntrinsicResolver>);

    ~ModuleRepositoryImpl() {

    }

    outcome::result<std::shared_ptr<ModuleInstance>> getInstanceAt(
        std::shared_ptr<RuntimeCodeProvider> code_provider,
        const primitives::BlockInfo &block) override;

    outcome::result<std::shared_ptr<ModuleInstance>> getInstanceAtLatest(
        std::shared_ptr<RuntimeCodeProvider> code_provider) override;

    outcome::result<std::unique_ptr<Module>> loadFrom(
        gsl::span<const uint8_t> byte_code) override;

   private:
    // just a helper to avoid duplicating logic in getInstanceAt and
    // getInstanceAtLatest
    outcome::result<std::shared_ptr<ModuleInstance>> getInstanceAt_Internal(
        gsl::span<const uint8_t> code, storage::trie::RootHash state);

    std::unordered_map<common::Hash256, std::shared_ptr<Module>> modules_;
    std::unordered_map<common::Hash256, std::shared_ptr<ModuleInstance>>
        instances_;
    WAVM::Runtime::GCPointer<WAVM::Runtime::Compartment> compartment_;
    std::shared_ptr<IntrinsicResolver> resolver_;
    std::shared_ptr<const crypto::Hasher> hasher_;
    log::Logger logger_;
  };

}  // namespace kagome::runtime::wavm

#endif  // KAGOME_CORE_RUNTIME_WAVM_IMPL_MODULE_REPOSITORY_IMPL_HPP
