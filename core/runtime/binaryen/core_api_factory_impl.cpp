/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/binaryen/core_api_factory_impl.hpp"

#include "runtime/binaryen/binaryen_memory_provider.hpp"
#include "runtime/binaryen/instance_environment_factory.hpp"
#include "runtime/binaryen/module/module_impl.hpp"
#include "runtime/common/constant_code_provider.hpp"
#include "runtime/common/runtime_properties_cache_impl.hpp"
#include "runtime/common/trie_storage_provider_impl.hpp"
#include "runtime/executor.hpp"
#include "runtime/module_repository.hpp"
#include "runtime/runtime_api/impl/core.hpp"

namespace kagome::runtime::binaryen {

  CoreApiFactoryImpl::CoreApiFactoryImpl(
      std::shared_ptr<const ModuleFactory> module_factory)
      : module_factory_(std::move(module_factory)) {
    BOOST_ASSERT(module_factory_ != nullptr);
  }

  outcome::result<std::unique_ptr<RestrictedCore>> CoreApiFactoryImpl::make(
      std::shared_ptr<const crypto::Hasher> hasher,
      const std::vector<uint8_t> &runtime_code) const {
    OUTCOME_TRY(
        ctx, RuntimeContextFactory::fromCode(*module_factory_, runtime_code));
    return std::make_unique<RestrictedCoreImpl>(std::move(ctx));
  }

}  // namespace kagome::runtime::binaryen
