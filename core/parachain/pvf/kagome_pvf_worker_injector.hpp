/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

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
#include "runtime/common/core_api_factory_impl.hpp"
#include "runtime/common/runtime_properties_cache_impl.hpp"
#include "runtime/memory_provider.hpp"
#include "runtime/module.hpp"
#include "storage/trie/serialization/trie_serializer_impl.hpp"
#include "storage/trie/trie_storage.hpp"

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

using kagome::injector::bind_by_lambda;

#if KAGOME_WASM_COMPILER_WAVM == 0 && KAGOME_WASM_COMPILER_WASM_EDGE == 0
#error "No WASM compiler defined"
#endif

namespace kagome::parachain {
  struct NullTrieStorage : storage::trie::TrieStorage {
    outcome::result<std::unique_ptr<storage::trie::TrieBatch>>
    getPersistentBatchAt(const storage::trie::RootHash &root,
                         TrieChangesTrackerOpt changes_tracker) override {
      return nullptr;
    }
    outcome::result<std::unique_ptr<storage::trie::TrieBatch>>
    getEphemeralBatchAt(const storage::trie::RootHash &root) const override {
      return nullptr;
    }
    outcome::result<std::unique_ptr<storage::trie::TrieBatch>>
    getProofReaderBatchAt(const storage::trie::RootHash &root,
                          const OnNodeLoaded &on_node_loaded) const override {
      return nullptr;
    }
  };

  template <typename T>
  auto bind_null() {
    return bind_by_lambda<T>([](auto &) { return nullptr; });
  }

  template <typename T>
  using sptr = std::shared_ptr<T>;
  auto pvf_worker_injector(const PvfWorkerInputConfig &input) {
    namespace di = boost::di;
    return di::make_injector(
        di::bind<crypto::Hasher>.to<crypto::HasherImpl>(),
        di::bind<crypto::EcdsaProvider>.to<crypto::EcdsaProviderImpl>(),
        di::bind<crypto::Ed25519Provider>.to<crypto::Ed25519ProviderImpl>(),
        di::bind<crypto::Sr25519Provider>.to<crypto::Sr25519ProviderImpl>(),
        di::bind<crypto::Bip39Provider>.to<crypto::Bip39ProviderImpl>(),
        di::bind<crypto::Pbkdf2Provider>.to<crypto::Pbkdf2ProviderImpl>(),
        di::bind<crypto::Secp256k1Provider>.to<crypto::Secp256k1ProviderImpl>(),
        bind_null<crypto::EllipticCurves>(),
        bind_null<crypto::KeyStore>(),
        bind_null<offchain::OffchainPersistentStorage>(),
        bind_null<offchain::OffchainWorkerPool>(),
        di::bind<runtime::CoreApiFactory>.to<runtime::CoreApiFactoryImpl>(),
        di::bind<host_api::HostApiFactory>.to<host_api::HostApiFactoryImpl>(),
        di::bind<storage::trie::TrieStorage>.to<NullTrieStorage>(),
        bind_null<runtime::RuntimeInstancesPool>(),
        bind_null<storage::trie::TrieSerializer>()

#if KAGOME_WASM_COMPILER_WAVM == 1
            ,
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
        di::bind<runtime::ModuleFactory>()
            .to<runtime::wavm::ModuleFactoryImpl>()
#endif

#if KAGOME_WASM_COMPILER_WASM_EDGE == 1
            ,
        bind_by_lambda<runtime::wasm_edge::ModuleFactoryImpl::Config>(
            [engine = input.engine](const auto &injector) {
              using E = runtime::wasm_edge::ModuleFactoryImpl::ExecType;
              runtime::wasm_edge::ModuleFactoryImpl::Config config{
                  engine == RuntimeEngine::kWasmEdgeCompiled ? E::Compiled
                                                             : E::Interpreted,
              };
              return std::make_shared<decltype(config)>(config);
            }),
        di::bind<runtime::ModuleFactory>.template to<runtime::wasm_edge::ModuleFactoryImpl>()
#endif
    );
  }
}  // namespace kagome::parachain
