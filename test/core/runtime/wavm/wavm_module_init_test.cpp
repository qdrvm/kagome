/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <filesystem>
#include <string_view>

#include <gtest/gtest.h>
#include <rocksdb/options.h>
#include <libp2p/crypto/random_generator/boost_generator.hpp>

#include "blockchain/impl/block_header_repository_impl.hpp"
#include "core/runtime/wavm/runtime_paths.hpp"
#include "crypto/bandersnatch/bandersnatch_provider_impl.hpp"
#include "crypto/bip39/impl/bip39_provider_impl.hpp"
#include "crypto/ecdsa/ecdsa_provider_impl.hpp"
#include "crypto/ed25519/ed25519_provider_impl.hpp"
#include "crypto/hasher/hasher_impl.hpp"
#include "crypto/key_store/key_store_impl.hpp"
#include "crypto/pbkdf2/impl/pbkdf2_provider_impl.hpp"
#include "crypto/secp256k1/secp256k1_provider_impl.hpp"
#include "crypto/sr25519/sr25519_provider_impl.hpp"
#include "host_api/impl/host_api_factory_impl.hpp"
#include "mock/core/application/app_state_manager_mock.hpp"
#include "offchain/impl/offchain_persistent_storage.hpp"
#include "offchain/impl/offchain_worker_pool_impl.hpp"
#include "runtime/common/runtime_properties_cache_impl.hpp"
#include "runtime/executor.hpp"
#include "runtime/memory_provider.hpp"
#include "runtime/module.hpp"
#include "runtime/runtime_context.hpp"
#include "runtime/wavm/compartment_wrapper.hpp"
#include "runtime/wavm/instance_environment_factory.hpp"
#include "runtime/wavm/intrinsics/intrinsic_module.hpp"
#include "runtime/wavm/module_factory_impl.hpp"
#include "runtime/wavm/module_params.hpp"
#include "storage/in_memory/in_memory_spaced_storage.hpp"
#include "storage/rocksdb/rocksdb.hpp"
#include "storage/trie/impl/trie_storage_backend_impl.hpp"
#include "storage/trie/impl/trie_storage_impl.hpp"
#include "storage/trie/polkadot_trie/polkadot_trie_factory_impl.hpp"
#include "storage/trie/serialization/polkadot_codec.hpp"
#include "storage/trie/serialization/trie_serializer_impl.hpp"
#include "storage/trie_pruner/impl/dummy_pruner.hpp"
#include "testutil/outcome.hpp"
#include "testutil/prepare_loggers.hpp"
#include "testutil/runtime/common/basic_code_provider.hpp"

class WavmModuleInitTest : public ::testing::TestWithParam<std::string_view> {
 public:
  static void SetUpTestCase() {
    testutil::prepareLoggers();
  }

  void SetUp() override {
    std::filesystem::path base_path{std::filesystem::temp_directory_path()
                                    / "wasm_module_init_test"};

    auto compartment =
        std::make_shared<kagome::runtime::wavm::CompartmentWrapper>(
            "WAVM Compartment");
    auto module_params =
        std::make_shared<kagome::runtime::wavm::ModuleParams>();

    auto trie_factory =
        std::make_shared<kagome::storage::trie::PolkadotTrieFactoryImpl>();
    auto codec = std::make_shared<kagome::storage::trie::PolkadotCodec>();
    auto storage = std::make_shared<kagome::storage::InMemorySpacedStorage>();
    auto node_storage_backend =
        std::make_shared<kagome::storage::trie::TrieStorageBackendImpl>(
            storage);
    auto serializer =
        std::make_shared<kagome::storage::trie::TrieSerializerImpl>(
            trie_factory, codec, node_storage_backend);
    auto state_pruner =
        std::make_shared<kagome::storage::trie_pruner::DummyPruner>();
    std::shared_ptr<kagome::storage::trie::TrieStorageImpl> trie_storage =
        kagome::storage::trie::TrieStorageImpl::createEmpty(
            trie_factory, codec, serializer, state_pruner)
            .value();
    auto intrinsic_module =
        std::make_shared<kagome::runtime::wavm::IntrinsicModule>(
            compartment, module_params->intrinsicMemoryType);

    using namespace kagome::crypto;
    auto hasher = std::make_shared<HasherImpl>();
    auto csprng =
        std::make_shared<libp2p::crypto::random::BoostRandomGenerator>();
    auto ecdsa_provider = std::make_shared<EcdsaProviderImpl>(hasher);
    auto ed25519_provider = std::make_shared<Ed25519ProviderImpl>(hasher);
    auto sr25519_provider = std::make_shared<Sr25519ProviderImpl>();
    auto bandersnatch_provider = std::make_shared<BandersnatchProviderImpl>();

    auto secp256k1_provider = std::make_shared<Secp256k1ProviderImpl>();
    auto pbkdf2_provider = std::make_shared<Pbkdf2ProviderImpl>();
    auto bip39_provider =
        std::make_shared<Bip39ProviderImpl>(std::move(pbkdf2_provider), hasher);
    std::shared_ptr key_file_storage =
        kagome::crypto::KeyFileStorage::createAt(base_path).value();
    KeyStore::Config config{base_path};
    auto key_store = std::make_shared<KeyStore>(
        std::make_unique<KeySuiteStoreImpl<Sr25519Provider>>(
            std::move(sr25519_provider),
            bip39_provider,
            csprng,
            key_file_storage),
        std::make_unique<KeySuiteStoreImpl<Ed25519Provider>>(
            ed25519_provider,  //
            bip39_provider,
            csprng,
            key_file_storage),
        std::make_unique<KeySuiteStoreImpl<EcdsaProvider>>(
            std::move(ecdsa_provider),
            bip39_provider,
            csprng,
            key_file_storage),
        std::make_unique<KeySuiteStoreImpl<BandersnatchProvider>>(
            std::move(bandersnatch_provider),
            bip39_provider,
            csprng,
            key_file_storage),
        ed25519_provider,
        std::make_shared<kagome::application::AppStateManagerMock>(),
        config);

    rocksdb::Options db_options{};
    db_options.create_if_missing = true;
    std::shared_ptr<kagome::storage::RocksDb> database =
        kagome::storage::RocksDb::create(base_path / "db", db_options).value();

    auto header_repo =
        std::make_shared<kagome::blockchain::BlockHeaderRepositoryImpl>(
            database, hasher);
    auto offchain_persistent_storage =
        std::make_shared<kagome::offchain::OffchainPersistentStorageImpl>(
            database);
    auto offchain_worker_pool =
        std::make_shared<kagome::offchain::OffchainWorkerPoolImpl>();

    auto host_api_factory =
        std::make_shared<kagome::host_api::HostApiFactoryImpl>(
            kagome::host_api::OffchainExtensionConfig{},
            ecdsa_provider,
            ed25519_provider,
            sr25519_provider,
            bandersnatch_provider,
            secp256k1_provider,
            hasher,
            key_store,
            offchain_persistent_storage,
            offchain_worker_pool);

    auto cache =
        std::make_shared<kagome::runtime::RuntimePropertiesCacheImpl>();

    module_factory_ =
        std::make_shared<kagome::runtime::wavm::ModuleFactoryImpl>(
            compartment,
            module_params,
            host_api_factory,
            trie_storage,
            serializer,
            intrinsic_module,
            std::nullopt,
            hasher);

    auto instance_env_factory = std::make_shared<
        const kagome::runtime::wavm::InstanceEnvironmentFactory>(
        trie_storage, serializer, host_api_factory, module_factory_);
  }

  std::shared_ptr<kagome::runtime::ModuleFactory> module_factory_;
  kagome::log::Logger log_ = kagome::log::createLogger("Test");
};

TEST_P(WavmModuleInitTest, DISABLED_SingleModule) {
  auto wasm = GetParam();
  SL_INFO(log_, "Trying {}", wasm);
  auto code_provider = std::make_shared<kagome::runtime::BasicCodeProvider>(
      std::string(kBasePath) + std::string(wasm));
  EXPECT_OUTCOME_TRUE(code, code_provider->getCodeAt({}));
  EXPECT_OUTCOME_TRUE(runtime_context,
                      kagome::runtime::RuntimeContextFactory::fromCode(
                          *module_factory_, *code, {}));
  EXPECT_OUTCOME_TRUE(response,
                      runtime_context.module_instance->callExportFunction(
                          runtime_context, "Core_version", {}));
  auto memory = runtime_context.module_instance->getEnvironment()
                    .memory_provider->getCurrentMemory();
  GTEST_ASSERT_TRUE(memory.has_value());
  EXPECT_OUTCOME_TRUE(cv, scale::decode<kagome::primitives::Version>(response));
  SL_INFO(log_,
          "Module initialized - spec {}-{}, impl {}-{}",
          cv.spec_name,
          cv.spec_version,
          cv.impl_name,
          cv.impl_version);
}

INSTANTIATE_TEST_SUITE_P(SingleModule,
                         WavmModuleInitTest,
                         testing::ValuesIn(kKusamaParachains));
