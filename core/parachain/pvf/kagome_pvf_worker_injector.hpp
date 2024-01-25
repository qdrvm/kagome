/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

// #define BOOST_DI_CFG_CTOR_LIMIT_SIZE 32
#include <boost/di.hpp>

#include "blockchain/block_header_repository.hpp"
#include "crypto/bip39/impl/bip39_provider_impl.hpp"
#include "crypto/ecdsa/ecdsa_provider_impl.hpp"
#include "crypto/ed25519/ed25519_provider_impl.hpp"
#include "crypto/hasher/hasher_impl.hpp"
#include "crypto/pbkdf2/impl/pbkdf2_provider_impl.hpp"
#include "crypto/secp256k1/secp256k1_provider_impl.hpp"
#include "crypto/sr25519/sr25519_provider_impl.hpp"
#include "host_api/impl/host_api_factory_impl.hpp"
#include "injector/bind_by_lambda.hpp"
#include "parachain/pvf/pvf_worker_types.hpp"
#include "runtime/binaryen/instance_environment_factory.hpp"
#include "runtime/binaryen/module/module_factory_impl.hpp"
#include "runtime/common/runtime_properties_cache_impl.hpp"
#include "runtime/memory_provider.hpp"
#include "runtime/module.hpp"

#if KAGOME_WASM_COMPILER_WAVM == 1
#include "runtime/wavm/compartment_wrapper.hpp"
#include "runtime/wavm/instance_environment_factory.hpp"
#include "runtime/wavm/intrinsics/intrinsic_functions.hpp"
#include "runtime/wavm/intrinsics/intrinsic_module.hpp"
#include "runtime/wavm/module_factory_impl.hpp"
#include "runtime/wavm/module_params.hpp"
#endif

#if KAGOME_WASM_COMPILER_WASM_EDGE == 1
#include "runtime/wasm_edge/module_factory_impl.hpp"
#endif

#include "storage/trie/serialization/trie_serializer_impl.hpp"

using kagome::injector::bind_by_lambda;

#if KAGOME_WASM_COMPILER_WAVM == 0 && KAGOME_WASM_COMPILER_WASM_EDGE == 0
  #error "No WASM compiler defined"
#endif

namespace kagome::parachain {
  template <typename T>
  auto bind_null() {
    return bind_by_lambda<T>([](auto &) { return nullptr; });
  }

  template <typename T>
  using sptr = std::shared_ptr<T>;
  auto pvf_worker_injector(const PvfWorkerInput &input) {
    namespace di = boost::di;
    return di::make_injector(
        di::bind<crypto::Hasher>.to<crypto::HasherImpl>(),
        di::bind<crypto::EcdsaProvider>.to<crypto::EcdsaProviderImpl>(),
        di::bind<crypto::Ed25519Provider>.to<crypto::Ed25519ProviderImpl>(),
        di::bind<crypto::Sr25519Provider>.to<crypto::Sr25519ProviderImpl>(),
        di::bind<crypto::Bip39Provider>.to<crypto::Bip39ProviderImpl>(),
        di::bind<crypto::Pbkdf2Provider>.to<crypto::Pbkdf2ProviderImpl>(),
        di::bind<crypto::Secp256k1Provider>.to<crypto::Secp256k1ProviderImpl>(),

        bind_null<crypto::CryptoStore>(),
        bind_null<offchain::OffchainPersistentStorage>(),
        bind_null<offchain::OffchainWorkerPool>(),
        // // bind_null<host_api::HostApiFactory>(),
        di::bind<host_api::HostApiFactory>.to<host_api::HostApiFactoryImpl>(),
        bind_null<storage::trie::TrieSerializer>(),
        // // bind_null<runtime::RuntimePropertiesCache>(),
        // di::bind<runtime::RuntimePropertiesCache>.to<runtime::RuntimePropertiesCacheImpl>(),
        // bind_null<blockchain::BlockHeaderRepository>(),
        bind_null<storage::trie::TrieStorage>()
        // di::bind<runtime::SingleModuleCache>().to<runtime::SingleModuleCache>()

    // di::bind<runtime::binaryen::InstanceEnvironmentFactory>().to<runtime::binaryen::InstanceEnvironmentFactory>(),
    // bind_null<runtime::binaryen::InstanceEnvironmentFactory>(),

#if KAGOME_WASM_COMPILER_WAVM == 1
            ,
#warning "WAVM"
        bind_by_lambda<runtime::wavm::CompartmentWrapper>([](auto &injector) {
          return std::make_shared<runtime::wavm::CompartmentWrapper>(
              "Runtime Compartment");
        }),
        bind_by_lambda<runtime::wavm::IntrinsicModule>(
            [](const auto &injector) {
              auto compartment = injector.template create<
                  std::shared_ptr<runtime::wavm::CompartmentWrapper>>();
              runtime::wavm::ModuleParams module_params{};
              auto module = std::make_shared<runtime::wavm::IntrinsicModule>(
                  compartment, module_params.intrinsicMemoryType);
              runtime::wavm::registerHostApiMethods(*module);
              return module;
            }),
        // to remove
        // bind_by_lambda<
        //     runtime::binaryen::InstanceEnvironmentFactory>([](const auto
        //                                                           &injector)
        //                                                           {
        //   // clang-format off
        //     return
        //     std::make_shared<runtime::binaryen::InstanceEnvironmentFactory>(
        //       injector.template create<sptr<storage::trie::TrieStorage>>(),
        //       injector.template
        //       create<sptr<storage::trie::TrieSerializer>>(),
        //       injector.template create<sptr<host_api::HostApiFactory>>(),
        //       injector.template
        //       create<sptr<blockchain::BlockHeaderRepository>>(),
        //       injector.template
        //       create<sptr<runtime::RuntimePropertiesCache>>()
        //     );
        // }),

        bind_by_lambda<runtime::wavm::ModuleFactoryImpl>([](const auto
                                                                &injector) {
          std::optional<std::shared_ptr<runtime::wavm::ModuleCache>>
              module_cache_opt;
          //   if (app_config.useWavmCache()) {
          //     module_cache_opt =
          //     std::make_shared<runtime::wavm::ModuleCache>(
          //         injector.template create<sptr<crypto::Hasher>>(),
          //         app_config.runtimeCacheDirPath());
          //   }
          // clang-format off
          return std::make_shared<runtime::wavm::ModuleFactoryImpl>(
            //<kagome::offchain::OffchainPersistentStorage>
              injector.template create<sptr<runtime::wavm::CompartmentWrapper>>(),
              injector.template create<sptr<runtime::wavm::ModuleParams>>(),
              injector.template create<sptr<runtime::wavm::InstanceEnvironmentFactory>>(),
              injector.template create<sptr<runtime::wavm::IntrinsicModule>>(),
              module_cache_opt,
              injector.template create<sptr<crypto::Hasher>>()
              );
        })
#warning "WAVM!"
      #endif // WAVM

      #if KAGOME_WASM_COMPILER_WASM_EDGE == 1
      ,
        di::bind<runtime::ModuleFactory>.template to<runtime::wasm_edge::ModuleFactoryImpl>()
        #warning "WASMEDGE"
      #endif

        //
    );
  }
}  // namespace kagome::parachain
