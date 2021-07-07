/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/wavm/executor.hpp"

#include <gtest/gtest.h>

#include <boost/filesystem.hpp>

#include "crypto/bip39/impl/bip39_provider_impl.hpp"
#include "crypto/crypto_store/crypto_store_impl.hpp"
#include "crypto/ed25519/ed25519_provider_impl.hpp"
#include "crypto/hasher/hasher_impl.hpp"
#include "crypto/pbkdf2/impl/pbkdf2_provider_impl.hpp"
#include "crypto/random_generator/boost_generator.hpp"
#include "crypto/secp256k1/secp256k1_provider_impl.hpp"
#include "crypto/sr25519/sr25519_provider_impl.hpp"
#include "host_api/impl/host_api_factory_impl.hpp"
#include "mock/core/blockchain/block_header_repository_mock.hpp"
#include "mock/core/runtime/runtime_upgrade_tracker_mock.hpp"
#include "mock/core/storage/changes_trie/changes_tracker_mock.hpp"
#include "runtime/common/trie_storage_provider_impl.hpp"
#include "runtime/wavm/impl/compartment_wrapper.hpp"
#include "runtime/wavm/impl/core_api_factory.hpp"
#include "runtime/wavm/impl/intrinsic_module_instance.hpp"
#include "runtime/wavm/impl/intrinsic_resolver_impl.hpp"
#include "runtime/wavm/impl/module_repository_impl.hpp"
#include "storage/in_memory/in_memory_storage.hpp"
#include "storage/trie/impl/trie_storage_backend_impl.hpp"
#include "storage/trie/impl/trie_storage_impl.hpp"
#include "storage/trie/polkadot_trie/polkadot_trie_factory_impl.hpp"
#include "storage/trie/serialization/polkadot_codec.hpp"
#include "storage/trie/serialization/trie_serializer_impl.hpp"
#include "testutil/literals.hpp"
#include "testutil/prepare_loggers.hpp"
#include "testutil/runtime/common/basic_wasm_provider.hpp"

using kagome::blockchain::BlockHeaderRepository;
using kagome::blockchain::BlockHeaderRepositoryMock;
using kagome::common::Buffer;
using kagome::crypto::Bip39ProviderImpl;
using kagome::crypto::BoostRandomGenerator;
using kagome::crypto::CryptoStoreImpl;
using kagome::crypto::Ed25519ProviderImpl;
using kagome::crypto::Ed25519Suite;
using kagome::crypto::HasherImpl;
using kagome::crypto::KeyFileStorage;
using kagome::crypto::Pbkdf2ProviderImpl;
using kagome::crypto::Secp256k1ProviderImpl;
using kagome::crypto::Sr25519ProviderImpl;
using kagome::crypto::Sr25519Suite;
using kagome::primitives::BlockHash;
using kagome::runtime::RuntimeCodeProvider;
using kagome::runtime::TrieStorageProvider;
using kagome::runtime::TrieStorageProviderImpl;
using kagome::runtime::wavm::Executor;
using kagome::storage::changes_trie::ChangesTrackerMock;
using kagome::storage::trie::PolkadotCodec;
using kagome::storage::trie::PolkadotTrieFactoryImpl;
using kagome::storage::trie::PolkadotTrieImpl;
using kagome::storage::trie::TrieSerializerImpl;
using kagome::storage::trie::TrieStorage;
using kagome::storage::trie::TrieStorageImpl;
using testing::Return;

namespace fs = boost::filesystem;

class WasmExecutorTest : public ::testing::Test {
 public:
  static void SetUpTestCase() {
    testutil::prepareLoggers();
  }

  void SetUp() override {
    // path to a file with wasm code in wasm/ subfolder
    auto wasm_path = fs::path(__FILE__).parent_path().parent_path().string()
                     + "/wasm/sumtwo.wasm";
    wasm_provider_ =
        std::make_shared<kagome::runtime::BasicCodeProvider>(wasm_path);

    auto backend =
        std::make_shared<kagome::storage::trie::TrieStorageBackendImpl>(
            std::make_shared<kagome::storage::InMemoryStorage>(),
            kagome::common::Buffer{});

    auto trie_factory = std::make_shared<PolkadotTrieFactoryImpl>();
    auto codec = std::make_shared<PolkadotCodec>();
    auto serializer =
        std::make_shared<TrieSerializerImpl>(trie_factory, codec, backend);

    auto trie_db = kagome::storage::trie::TrieStorageImpl::createEmpty(
                       trie_factory, codec, serializer, boost::none)
                       .value();

    storage_provider_ =
        std::make_shared<TrieStorageProviderImpl>(std::move(trie_db));

    auto random_generator = std::make_shared<BoostRandomGenerator>();
    auto sr25519_provider =
        std::make_shared<Sr25519ProviderImpl>(random_generator);
    auto ed25519_provider =
        std::make_shared<Ed25519ProviderImpl>(random_generator);
    auto secp256k1_provider = std::make_shared<Secp256k1ProviderImpl>();
    auto hasher = std::make_shared<HasherImpl>();
    auto pbkdf2_provider = std::make_shared<Pbkdf2ProviderImpl>();
    auto bip39_provider = std::make_shared<Bip39ProviderImpl>(pbkdf2_provider);

    auto keystore_path =
        boost::filesystem::temp_directory_path() / "kagome_keystore_test_dir";
    auto crypto_store = std::make_shared<CryptoStoreImpl>(
        std::make_shared<Ed25519Suite>(ed25519_provider),
        std::make_shared<Sr25519Suite>(sr25519_provider),
        bip39_provider,
        KeyFileStorage::createAt(keystore_path).value());
    auto changes_tracker =
        std::make_shared<kagome::storage::changes_trie::ChangesTrackerMock>();
    auto extension_factory =
        std::make_shared<kagome::host_api::HostApiFactoryImpl>(
            std::make_shared<ChangesTrackerMock>(),
            sr25519_provider,
            ed25519_provider,
            secp256k1_provider,
            hasher,
            crypto_store,
            bip39_provider);

    header_repo_ =
        std::make_shared<kagome::blockchain::BlockHeaderRepositoryMock>();

    auto compartment_wrapper =
        std::make_shared<kagome::runtime::wavm::CompartmentWrapper>(
            std::string("test_compartment"));
    auto intrinsic_module_instance =
        std::make_shared<kagome::runtime::wavm::IntrinsicModuleInstance>(
            compartment_wrapper);

    auto memory_provider =
        std::make_shared<kagome::runtime::wavm::WavmMemoryProvider>(
            intrinsic_module_instance);
    auto intrinsic_resolver =
        std::make_shared<kagome::runtime::wavm::IntrinsicResolverImpl>(
            intrinsic_module_instance, compartment_wrapper);
    runtime_upgrade_tracker_ =
        std::make_shared<kagome::runtime::RuntimeUpgradeTrackerMock>();
    auto module_repo =
        std::make_shared<kagome::runtime::wavm::ModuleRepositoryImpl>(
            compartment_wrapper,
            runtime_upgrade_tracker_,
            hasher,
            intrinsic_resolver);

    auto core_provider =
        std::make_shared<kagome::runtime::wavm::CoreApiFactory>(
            compartment_wrapper,
            intrinsic_module_instance,
            storage_provider_,
            header_repo_,
            changes_tracker,
            extension_factory);
    auto host_api = std::shared_ptr{extension_factory->make(
        core_provider, memory_provider, storage_provider_)};
    kagome::runtime::wavm::pushHostApi(host_api);

    executor_ = std::make_shared<Executor>(storage_provider_,
                                           memory_provider,
                                           module_repo,
                                           header_repo_,
                                           wasm_provider_);
    executor_->setHostApi(host_api);
  }

 protected:
  std::shared_ptr<Executor> executor_;
  std::shared_ptr<TrieStorageProvider> storage_provider_;
  std::shared_ptr<RuntimeCodeProvider> wasm_provider_;
  std::shared_ptr<BlockHeaderRepositoryMock> header_repo_;
  std::shared_ptr<kagome::runtime::RuntimeUpgradeTrackerMock>
      runtime_upgrade_tracker_;
};

/**
 * @given wasm executor
 * @when call is invoked with wasm code with addTwo function
 * @then proper result is returned
 */
TEST_F(WasmExecutorTest, DISABLED_ExecuteCode) {
  EXPECT_CALL(*header_repo_, getHashByNumber(0))
      .WillOnce(Return("blockhash0"_hash256));
  EXPECT_CALL(*header_repo_, getBlockHeader(kagome::primitives::BlockId{0}))
      .WillOnce(Return(
          kagome::primitives::BlockHeader{.parent_hash{},
                                          .number = 0,
                                          .state_root{"stateroot0"_hash256},
                                          .extrinsics_root{},
                                          .digest{}}));
  EXPECT_CALL(*runtime_upgrade_tracker_,
              getLastCodeUpdateState(
                  kagome::primitives::BlockInfo{0, "blockhash0"_hash256}))
      .WillOnce(Return(outcome::success("stateroot0"_hash256)));

  auto res = executor_->callAtLatest<int32_t>("addTwo", 1, 2);

  ASSERT_TRUE(res) << res.error().message();
  ASSERT_EQ(res.value(), 3);
}
