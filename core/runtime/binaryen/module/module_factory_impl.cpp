/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/binaryen/module/module_factory_impl.hpp"

#include "host_api/host_api_factory.hpp"
#include "runtime/binaryen/binaryen_memory_factory.hpp"
#include "runtime/binaryen/binaryen_memory_provider.hpp"
#include "runtime/binaryen/core_api_factory_impl.hpp"
#include "runtime/binaryen/instance_environment_factory.hpp"
#include "runtime/binaryen/module/module_impl.hpp"
#include "runtime/common/trie_storage_provider_impl.hpp"

namespace kagome::runtime::binaryen {

  ModuleFactoryImpl::ModuleFactoryImpl(
      std::shared_ptr<InstanceEnvironmentFactory> env_factory,
      std::shared_ptr<storage::trie::TrieStorage> storage,
      std::shared_ptr<crypto::Hasher> hasher)
      : env_factory_{std::move(env_factory)},
        storage_{std::move(storage)},
        hasher_(std::move(hasher)) {
    BOOST_ASSERT(env_factory_ != nullptr);
    BOOST_ASSERT(storage_ != nullptr);
  }

  outcome::result<std::shared_ptr<Module>, CompilationError> ModuleFactoryImpl::make(
      gsl::span<const uint8_t> code) const {
    std::vector<uint8_t> code_vec{code.begin(), code.end()};
    OUTCOME_TRY(module,
                ModuleImpl::createFromCode(
                    code_vec, env_factory_, hasher_->sha2_256(code)));
    return module;
  }

}  // namespace kagome::runtime::binaryen
