/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/binaryen/module/module_factory_impl.hpp"

#include "crypto/hasher.hpp"
#include "host_api/host_api_factory.hpp"
#include "runtime/binaryen/binaryen_memory_factory.hpp"
#include "runtime/binaryen/binaryen_memory_provider.hpp"
#include "runtime/binaryen/instance_environment_factory.hpp"
#include "runtime/binaryen/module/module_impl.hpp"
#include "runtime/common/core_api_factory_impl.hpp"
#include "runtime/common/trie_storage_provider_impl.hpp"

namespace kagome::runtime::binaryen {

  ModuleFactoryImpl::ModuleFactoryImpl(
      std::shared_ptr<InstanceEnvironmentFactory> env_factory,
      std::shared_ptr<storage::trie::TrieStorage> storage,
      std::shared_ptr<crypto::Hasher> hasher)
      : env_factory_{std::move(env_factory)},
        storage_{std::move(storage)},
        hasher_(std::move(hasher)) {}

  CompilationOutcome<std::shared_ptr<Module>> ModuleFactoryImpl::make(
      common::BufferView code) const {
    std::vector<uint8_t> code_vec{code.begin(), code.end()};
    OUTCOME_TRY(module,
                ModuleImpl::createFromCode(code_vec,
                                           env_factory_,
                                           shared_from_this(),
                                           hasher_->sha2_256(code)));
    return module;
  }

}  // namespace kagome::runtime::binaryen
