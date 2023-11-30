/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/wavm/module_factory_impl.hpp"

#include "crypto/hasher.hpp"
#include "runtime/wavm/instance_environment_factory.hpp"
#include "runtime/wavm/module.hpp"
#include "runtime/wavm/module_cache.hpp"
#include "runtime/wavm/module_params.hpp"

namespace kagome::runtime::wavm {

  ModuleFactoryImpl::ModuleFactoryImpl(
      std::shared_ptr<CompartmentWrapper> compartment,
      std::shared_ptr<ModuleParams> module_params,
      std::shared_ptr<host_api::HostApiFactory> host_api_factory,
      std::shared_ptr<storage::trie::TrieStorage> storage,
      std::shared_ptr<storage::trie::TrieSerializer> serializer,
      std::shared_ptr<IntrinsicModule> intrinsic_module,
      std::shared_ptr<runtime::SingleModuleCache> last_compiled_module,
      std::optional<std::shared_ptr<ModuleCache>> module_cache,
      std::shared_ptr<crypto::Hasher> hasher)
      : compartment_{std::move(compartment)},
        module_params_{std::move(module_params)},
        host_api_factory_{host_api_factory},
        storage_{storage},
        serializer_{serializer},
        last_compiled_module_{last_compiled_module},
        intrinsic_module_{std::move(intrinsic_module)},
        hasher_(std::move(hasher)) {
    BOOST_ASSERT(compartment_ != nullptr);
    BOOST_ASSERT(module_params_ != nullptr);
    BOOST_ASSERT(host_api_factory_ != nullptr);
    BOOST_ASSERT(storage_ != nullptr);
    BOOST_ASSERT(serializer_ != nullptr);
    BOOST_ASSERT(last_compiled_module_ != nullptr);
    BOOST_ASSERT(intrinsic_module_ != nullptr);
    BOOST_ASSERT(hasher_ != nullptr);

    if (module_cache.has_value()) {
      WAVM::Runtime::setGlobalObjectCache(std::move(module_cache.value()));
    }
  }

  outcome::result<std::shared_ptr<Module>, CompilationError>
  ModuleFactoryImpl::make(common::BufferView code) const {
    auto env_factory = std::make_shared<InstanceEnvironmentFactory>(
        storage_, serializer_, host_api_factory_, shared_from_this());
    OUTCOME_TRY(module,
                ModuleImpl::compileFrom(compartment_,
                                        *module_params_,
                                        intrinsic_module_,
                                        env_factory,
                                        code,
                                        hasher_->sha2_256(code)));
    return module;
  }

}  // namespace kagome::runtime::wavm
