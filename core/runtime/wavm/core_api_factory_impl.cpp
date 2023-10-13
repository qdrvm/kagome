/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/wavm/core_api_factory_impl.hpp"

#include "runtime/common/constant_code_provider.hpp"
#include "runtime/common/runtime_properties_cache_impl.hpp"
#include "runtime/common/trie_storage_provider_impl.hpp"
#include "runtime/executor.hpp"
#include "runtime/module_repository.hpp"
#include "runtime/runtime_api/impl/core.hpp"
#include "runtime/runtime_context.hpp"
#include "runtime/wavm/compartment_wrapper.hpp"
#include "runtime/wavm/instance_environment_factory.hpp"
#include "runtime/wavm/intrinsics/intrinsic_functions.hpp"
#include "runtime/wavm/intrinsics/intrinsic_module.hpp"
#include "runtime/wavm/intrinsics/intrinsic_resolver_impl.hpp"
#include "runtime/wavm/module.hpp"
#include "runtime/wavm/module_instance.hpp"
#include "runtime/wavm/module_params.hpp"

namespace kagome::runtime::wavm {

  CoreApiFactoryImpl::CoreApiFactoryImpl(
      std::shared_ptr<const ModuleFactory> module_factory,
      std::shared_ptr<SingleModuleCache> last_compiled_module)
      : module_factory_{module_factory}, last_compiled_module_{last_compiled_module} {
    BOOST_ASSERT(module_factory_);
    BOOST_ASSERT(last_compiled_module_);
  }

  outcome::result<std::unique_ptr<RestrictedCore>> CoreApiFactoryImpl::make(
      std::shared_ptr<const crypto::Hasher> hasher,
      const std::vector<uint8_t> &runtime_code) const {
    OUTCOME_TRY(
        ctx,
        RuntimeContextFactory::fromCode(*module_factory_, runtime_code, {}));
    return std::make_unique<RestrictedCoreImpl>(std::move(ctx));
  }

}  // namespace kagome::runtime::wavm
