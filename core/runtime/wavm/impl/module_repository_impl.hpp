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

namespace kagome::runtime {
  class RuntimeUpgradeTracker;
}

namespace kagome::runtime::wavm {

  class CompartmentWrapper;

  class ModuleRepositoryImpl final : public ModuleRepository {
   public:
    ModuleRepositoryImpl(
        std::shared_ptr<CompartmentWrapper> compartment,
        std::shared_ptr<const RuntimeUpgradeTracker> runtime_upgrade_tracker_,
        std::shared_ptr<const crypto::Hasher> hasher,
        std::shared_ptr<IntrinsicResolver>);

    outcome::result<std::shared_ptr<ModuleInstance>> getInstanceAt(
        std::shared_ptr<const RuntimeCodeProvider> code_provider,
        const primitives::BlockInfo &block) override;

    outcome::result<std::unique_ptr<Module>> loadFrom(
        gsl::span<const uint8_t> byte_code) override;

   private:
    std::shared_ptr<CompartmentWrapper> compartment_;
    std::shared_ptr<const RuntimeUpgradeTracker> runtime_upgrade_tracker_;
    std::unordered_map<storage::trie::RootHash, std::shared_ptr<Module>>
        modules_;
    std::unordered_map<storage::trie::RootHash, std::shared_ptr<ModuleInstance>>
        instances_;
    std::shared_ptr<IntrinsicResolver> resolver_;
    std::shared_ptr<const crypto::Hasher> hasher_;
    log::Logger logger_;
  };

}  // namespace kagome::runtime::wavm

#endif  // KAGOME_CORE_RUNTIME_WAVM_IMPL_MODULE_REPOSITORY_IMPL_HPP
