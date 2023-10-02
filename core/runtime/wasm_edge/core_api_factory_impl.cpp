/**
 * Copyright Quadrivium LLC All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/wasm_edge/core_api_factory_impl.hpp"

#include "runtime/executor.hpp"
#include "runtime/runtime_api/impl/core.hpp"

namespace kagome::runtime::wasm_edge {

  CoreApiFactoryImpl::CoreApiFactoryImpl(
      std::shared_ptr<const ModuleFactory> factory)
      : factory_{factory} {
    BOOST_ASSERT(factory_);
  }

  outcome::result<std::unique_ptr<RestrictedCore>> CoreApiFactoryImpl::make(
      std::shared_ptr<const crypto::Hasher> hasher,
      const std::vector<uint8_t> &runtime_code) const {
    OUTCOME_TRY(ctx,
                RuntimeContextFactory::fromCode(*factory_, runtime_code, {}));

    return std::make_unique<RestrictedCoreImpl>(std::move(ctx));
  }

}  // namespace kagome::runtime::wasm_edge