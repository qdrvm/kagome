/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "core_api_factory_impl.hpp"

#include "runtime/common/runtime_properties_cache_impl.hpp"
#include "runtime/common/trie_storage_provider_impl.hpp"
#include "runtime/module_repository.hpp"
#include "runtime/runtime_api/impl/core.hpp"
#include "runtime/runtime_context.hpp"

namespace kagome::runtime {

  CoreApiFactoryImpl::CoreApiFactoryImpl(
      std::shared_ptr<const ModuleFactory> module_factory)
      : module_factory_{module_factory} {
    BOOST_ASSERT(module_factory_);
  }

  outcome::result<std::unique_ptr<RestrictedCore>> CoreApiFactoryImpl::make(
      std::shared_ptr<const crypto::Hasher> hasher,
      const std::vector<uint8_t> &runtime_code) const {
    OUTCOME_TRY(
        ctx,
        RuntimeContextFactory::fromCode(*module_factory_, runtime_code, {}));
    return std::make_unique<RestrictedCoreImpl>(std::move(ctx));
  }

}  // namespace kagome::runtime
