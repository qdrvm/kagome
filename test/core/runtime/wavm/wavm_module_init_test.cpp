/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <fstream>
#include <gtest/gtest.h>
#include <rocksdb/options.h>
#include <libp2p/crypto/random_generator/boost_generator.hpp>
#include "blockchain/impl/block_header_repository_impl.hpp"  //header_repo
#include "crypto/bip39/impl/bip39_provider_impl.hpp"         //bip39_provider
#include "crypto/crypto_store/crypto_store_impl.hpp"         //crypto_store
#include "crypto/ecdsa/ecdsa_provider_impl.hpp"              //ecdsa_provider
#include "crypto/ed25519/ed25519_provider_impl.hpp"          //ed25519_provider
#include "crypto/hasher/hasher_impl.hpp"                     //hasher
#include "crypto/pbkdf2/impl/pbkdf2_provider_impl.hpp"       //pbkdf2_provider
#include "crypto/secp256k1/secp256k1_provider_impl.hpp"  //secp256k1_provider
#include "crypto/sr25519/sr25519_provider_impl.hpp"      //sr25519_provider
#include "host_api/impl/host_api_factory_impl.hpp"       // host_api_factory
#include "offchain/impl/offchain_persistent_storage.hpp"  //offchain_persistent_store
#include "offchain/impl/offchain_worker_pool_impl.hpp"  //offchain_worker_pool
#include "runtime/module.hpp"                           // smc
#include "runtime/runtime_api/impl/runtime_properties_cache_impl.hpp"  // cache
#include "runtime/wavm/compartment_wrapper.hpp"           // compartment
#include "runtime/wavm/instance_environment_factory.hpp"  // instance_env_factory
#include "runtime/wavm/intrinsics/intrinsic_module.hpp"   // intrinsic_module
#include "runtime/wavm/module_factory_impl.hpp"           // module_factory
#include "runtime/wavm/module_params.hpp"                 //module_params
#include "storage/in_memory/in_memory_storage.hpp"        // storage
#include "storage/rocksdb/rocksdb.hpp"                    //database
#include "storage/trie/impl/trie_storage_backend_impl.hpp"  // storage_backend
#include "storage/trie/impl/trie_storage_impl.hpp"          // trie_storage
#include "storage/trie/polkadot_trie/polkadot_trie_factory_impl.hpp"  // trie_factory
#include "storage/trie/serialization/polkadot_codec.hpp"              //codec
#include "storage/trie/serialization/trie_serializer_impl.hpp"  // serializer
#include "storage/trie_pruner/impl/dummy_pruner.hpp"            // trie_pruner
#include "testutil/outcome.hpp"
#include "testutil/prepare_loggers.hpp"
#include "testutil/runtime/common/basic_code_provider.hpp"  //code_provider_
#include "runtime/common/uncompress_code_if_needed.hpp"

class WavmModuleInitTest : public ::testing::Test {
 public:
  static void SetUpTestCase() {
    testutil::prepareLoggers();
  }

  void SetUp() override {
    code_provider_ = std::make_shared<kagome::runtime::BasicCodeProvider>(
        "/Users/igor/Downloads/Telegram "
        "Desktop/kusama-para-code/"
        "7b8c139e0ea5189beab1a3c32e5eb32582ab1178c6b38d3713669654dd73df8d."
        "wasm");

    auto compartment =
        std::make_shared<kagome::runtime::wavm::CompartmentWrapper>(
            "WAVM Compartment");
    auto module_params =
        std::make_shared<kagome::runtime::wavm::ModuleParams>();

    auto trie_factory =
        std::make_shared<kagome::storage::trie::PolkadotTrieFactoryImpl>();
    auto codec = std::make_shared<kagome::storage::trie::PolkadotCodec>();
    auto storage = std::make_shared<kagome::storage::InMemoryStorage>();
    auto storage_backend =
        std::make_shared<kagome::storage::trie::TrieStorageBackendImpl>(
            storage);
    auto serializer =
        std::make_shared<kagome::storage::trie::TrieSerializerImpl>(
            trie_factory, codec, storage_backend);
    auto state_pruner =
        std::make_shared<kagome::storage::trie_pruner::DummyPruner>();
    std::shared_ptr<kagome::storage::trie::TrieStorageImpl> trie_storage =
        kagome::storage::trie::TrieStorageImpl::createEmpty(
            trie_factory, codec, serializer, state_pruner)
            .value();
    auto intrinsic_module =
        std::make_shared<kagome::runtime::wavm::IntrinsicModule>(
            compartment, module_params->intrinsicMemoryType);

    auto hasher = std::make_shared<kagome::crypto::HasherImpl>();
    auto sr25519_provider =
        std::make_shared<kagome::crypto::Sr25519ProviderImpl>();
    auto ecdsa_provider =
        std::make_shared<kagome::crypto::EcdsaProviderImpl>(hasher);
    auto ed25519_provider =
        std::make_shared<kagome::crypto::Ed25519ProviderImpl>(hasher);
    auto secp256k1_provider =
        std::make_shared<kagome::crypto::Secp256k1ProviderImpl>();
    auto pbkdf2_provider =
        std::make_shared<kagome::crypto::Pbkdf2ProviderImpl>();
    auto bip39_provider = std::make_shared<kagome::crypto::Bip39ProviderImpl>(
        pbkdf2_provider, hasher);
    auto ecdsa_suite =
        std::make_shared<kagome::crypto::EcdsaSuite>(ecdsa_provider);
    auto ed_suite =
        std::make_shared<kagome::crypto::Ed25519Suite>(ed25519_provider);
    auto sr_suite =
        std::make_shared<kagome::crypto::Sr25519Suite>(sr25519_provider);
    std::shared_ptr<kagome::crypto::KeyFileStorage> key_fs =
        kagome::crypto::KeyFileStorage::createAt(
            "/tmp/kagome_vawm_tmp_key_storage")
            .value();
    auto csprng =
        std::make_shared<libp2p::crypto::random::BoostRandomGenerator>();
    auto crypto_store = std::make_shared<kagome::crypto::CryptoStoreImpl>(
        ecdsa_suite, ed_suite, sr_suite, bip39_provider, csprng, key_fs);

    rocksdb::Options db_options{};
    db_options.create_if_missing = true;
    std::shared_ptr<kagome::storage::RocksDb> database =
        kagome::storage::RocksDb::create("/tmp/kagome_tmp_db", db_options)
            .value();

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
            sr25519_provider,
            ecdsa_provider,
            ed25519_provider,
            secp256k1_provider,
            hasher,
            crypto_store,
            offchain_persistent_storage,
            offchain_worker_pool);

    auto smc = std::make_shared<kagome::runtime::SingleModuleCache>();
    auto cache =
        std::make_shared<kagome::runtime::RuntimePropertiesCacheImpl>();

    auto instance_env_factory = std::make_shared<
        const kagome::runtime::wavm::InstanceEnvironmentFactory>(
        trie_storage,
        serializer,
        compartment,
        module_params,
        intrinsic_module,
        host_api_factory,
        header_repo,
        smc,
        cache);

    module_factory_ =
        std::make_shared<kagome::runtime::wavm::ModuleFactoryImpl>(
            compartment,
            module_params,
            instance_env_factory,
            intrinsic_module,
            std::nullopt,
            hasher);
  }

  std::shared_ptr<kagome::runtime::BasicCodeProvider> code_provider_;
  std::shared_ptr<kagome::runtime::ModuleFactory> module_factory_;
};

TEST_F(WavmModuleInitTest, SingleModule) {
  EXPECT_OUTCOME_TRUE(code, code_provider_->getCodeAt({}));
  kagome::common::Buffer uncompressed_code;
  EXPECT_OUTCOME_TRUE_1(kagome::runtime::uncompressCodeIfNeeded(code, uncompressed_code));
  EXPECT_OUTCOME_TRUE(module, module_factory_->make(uncompressed_code));
  EXPECT_OUTCOME_TRUE(module_inst, module->instantiate());


//  std::ofstream stream("wasm.hex.txt");
////  auto v = uncompressed_code.toHex();
////  stream.write(v.c_str(), v.size());
////  stream.flush();
////  stream.close();
}
