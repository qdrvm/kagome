/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <gtest/gtest.h>

#include <memory>
#include <mock/libp2p/crypto/random_generator_mock.hpp>

#include "crypto/bandersnatch/bandersnatch_provider_impl.hpp"
#include "crypto/bip39/impl/bip39_provider_impl.hpp"
#include "crypto/ecdsa/ecdsa_provider_impl.hpp"
#include "crypto/ed25519/ed25519_provider_impl.hpp"
#include "crypto/elliptic_curves/elliptic_curves_impl.hpp"
#include "crypto/hasher/hasher_impl.hpp"
#include "crypto/key_store/key_store_impl.hpp"
#include "crypto/pbkdf2/impl/pbkdf2_provider_impl.hpp"
#include "crypto/random_generator/boost_generator.hpp"
#include "crypto/secp256k1/secp256k1_provider_impl.hpp"
#include "crypto/sr25519/sr25519_provider_impl.hpp"
#include "filesystem/common.hpp"
#include "host_api/impl/host_api_factory_impl.hpp"
#include "mock/core/application/app_configuration_mock.hpp"
#include "mock/core/application/app_state_manager_mock.hpp"
#include "mock/core/blockchain/block_tree_mock.hpp"
#include "mock/core/offchain/offchain_persistent_storage_mock.hpp"
#include "mock/core/offchain/offchain_worker_pool_mock.hpp"
#include "mock/core/runtime/runtime_properties_cache_mock.hpp"
#include "mock/core/runtime/trie_storage_provider_mock.hpp"
#include "mock/core/storage/spaced_storage_mock.hpp"
#include "mock/core/storage/trie/polkadot_trie_cursor_mock.h"
#include "mock/core/storage/trie/serialization/trie_serializer_mock.hpp"
#include "mock/core/storage/trie/trie_batches_mock.hpp"
#include "mock/core/storage/trie/trie_storage_mock.hpp"
#include "primitives/block.hpp"
#include "primitives/block_header.hpp"
#include "primitives/block_id.hpp"
#include "runtime/common/module_repository_impl.hpp"
#include "runtime/common/runtime_execution_error.hpp"
#include "runtime/common/runtime_instances_pool.hpp"
#include "runtime/common/runtime_upgrade_tracker_impl.hpp"
#include "runtime/core_api_factory.hpp"
#include "runtime/executor.hpp"
#include "runtime/module.hpp"
#include "runtime/runtime_context.hpp"
#include "runtime/wabt/instrument.hpp"
#include "storage/in_memory/in_memory_storage.hpp"
#include "testutil/literals.hpp"
#include "testutil/runtime/common/basic_code_provider.hpp"

using kagome::application::AppConfigurationMock;
using kagome::runtime::RuntimeInstancesPoolImpl;
using testing::_;
using testing::Invoke;
using testing::Return;

using namespace kagome;

class RuntimeTestBaseImpl {
 public:
  using Buffer = common::Buffer;
  using Block = primitives::Block;
  using BlockId = primitives::BlockId;
  using BlockHeader = primitives::BlockHeader;
  using Extrinsic = primitives::Extrinsic;
  using Digest = primitives::Digest;
  using TrieBatchMock = storage::trie::TrieBatchMock;

  void initStorage() {
    using storage::trie::TrieBatch;

    auto random_generator = std::make_shared<crypto::BoostRandomGenerator>();
    hasher_ = std::make_shared<crypto::HasherImpl>();
    auto sr25519_provider = std::make_shared<crypto::Sr25519ProviderImpl>();
    auto bandersnatch_provider =
        std::make_shared<crypto::BandersnatchProviderImpl>(hasher_);
    auto ecdsa_provider = std::make_shared<crypto::EcdsaProviderImpl>(hasher_);
    auto ed25519_provider =
        std::make_shared<crypto::Ed25519ProviderImpl>(hasher_);
    auto secp256k1_provider = std::make_shared<crypto::Secp256k1ProviderImpl>();
    auto elliptic_curves = std::make_shared<crypto::EllipticCurvesImpl>();
    auto pbkdf2_provider = std::make_shared<crypto::Pbkdf2ProviderImpl>();
    auto bip39_provider = std::make_shared<crypto::Bip39ProviderImpl>(
        pbkdf2_provider,
        std::make_shared<libp2p::crypto::random::CSPRNGMock>(),
        hasher_);
    auto keystore_path =
        filesystem::temp_directory_path() / filesystem::unique_path();
    std::shared_ptr key_file_storage =
        kagome::crypto::KeyFileStorage::createAt(keystore_path).value();
    crypto::KeyStore::Config config{keystore_path};
    auto key_store = std::make_shared<crypto::KeyStore>(
        std::make_unique<crypto::KeySuiteStoreImpl<crypto::Sr25519Provider>>(
            sr25519_provider,
            bip39_provider,
            random_generator,
            key_file_storage),
        std::make_unique<crypto::KeySuiteStoreImpl<crypto::Ed25519Provider>>(
            ed25519_provider,
            bip39_provider,
            random_generator,
            key_file_storage),
        std::make_unique<crypto::KeySuiteStoreImpl<crypto::EcdsaProvider>>(
            ecdsa_provider,  //
            bip39_provider,
            random_generator,
            key_file_storage),
        std::make_unique<
            crypto::KeySuiteStoreImpl<crypto::BandersnatchProvider>>(
            bandersnatch_provider,
            bip39_provider,
            random_generator,
            key_file_storage),
        ed25519_provider,
        std::make_shared<kagome::application::AppStateManagerMock>(),
        config);
    offchain_storage_ =
        std::make_shared<offchain::OffchainPersistentStorageMock>();
    offchain_worker_pool_ =
        std::make_shared<offchain::OffchainWorkerPoolMock>();

    host_api_factory_ = std::make_shared<host_api::HostApiFactoryImpl>(
        kagome::host_api::OffchainExtensionConfig{},
        ecdsa_provider,
        ed25519_provider,
        sr25519_provider,
        bandersnatch_provider,
        secp256k1_provider,
        elliptic_curves,
        hasher_,
        key_store,
        offchain_storage_,
        offchain_worker_pool_);

    block_tree_ =
        std::make_shared<testing::NiceMock<blockchain::BlockTreeMock>>();

    ON_CALL(*block_tree_, getHashByNumber(0))
        .WillByDefault(testing::Return("genesis_hash"_hash256));
    EXPECT_CALL(*block_tree_, getBlockHeader("genesis_hash"_hash256))
        .WillRepeatedly(testing::Return(primitives::BlockHeader{
            0,                             // number
            {},                            // parent_hash
            "genesis state root"_hash256,  // state_root
            "genesis ext root"_hash256,    // extrinsics_root
            {},                            // digest
        }));
  }

  virtual std::shared_ptr<runtime::ModuleFactory> createModuleFactory() = 0;

  void SetUpImpl() {
    initStorage();
    trie_storage_ = std::make_shared<storage::trie::TrieStorageMock>();
    serializer_ = std::make_shared<storage::trie::TrieSerializerMock>();

    auto buffer_storage = std::make_shared<storage::InMemoryStorage>();
    auto spaced_storage = std::make_shared<storage::SpacedStorageMock>();
    EXPECT_CALL(*spaced_storage, getSpace(_))
        .WillRepeatedly(Return(buffer_storage));

    cache_ = std::make_shared<runtime::RuntimePropertiesCacheMock>();
    ON_CALL(*cache_, getVersion(_, _))
        .WillByDefault(
            Invoke([](const auto &hash, auto func) { return func(); }));
    ON_CALL(*cache_, getMetadata(_, _))
        .WillByDefault(
            Invoke([](const auto &hash, auto func) { return func(); }));

    auto module_factory = createModuleFactory();

    auto wasm_path = kagome::filesystem::path(__FILE__).parent_path().string()
                   + "/wasm/sub2dev.wasm";
    wasm_provider_ = std::make_shared<runtime::BasicCodeProvider>(wasm_path);

    std::shared_ptr<runtime::RuntimeUpgradeTrackerImpl> upgrade_tracker =
        runtime::RuntimeUpgradeTrackerImpl::create(
            spaced_storage,
            std::make_shared<primitives::CodeSubstituteBlockIds>(),
            block_tree_)
            .value();

    auto wasm_cache_dir =
        filesystem::temp_directory_path() / "runtime_test_base_cache";
    std::filesystem::create_directories(wasm_cache_dir);
    EXPECT_CALL(app_config_, runtimeCacheDirPath())
        .WillOnce(Return(wasm_cache_dir));
    instance_pool_ = std::make_shared<RuntimeInstancesPoolImpl>(
        app_config_,
        module_factory,
        std::make_shared<runtime::WasmInstrumenter>());
    auto module_repo =
        std::make_shared<runtime::ModuleRepositoryImpl>(instance_pool_,
                                                        hasher_,
                                                        block_tree_,
                                                        upgrade_tracker,
                                                        trie_storage_,
                                                        module_factory,
                                                        wasm_provider_);

    ctx_factory_ = std::make_shared<runtime::RuntimeContextFactoryImpl>(
        module_repo, block_tree_);

    executor_ = std::make_shared<runtime::Executor>(ctx_factory_, cache_);
  }

  void preparePersistentStorageExpects() {
    EXPECT_CALL(*trie_storage_, getPersistentBatchAt(_, _))
        .WillOnce(testing::Invoke([this](auto &root, auto &&) {
          auto batch = std::make_unique<TrieBatchMock>();
          prepareStorageBatchExpectations(*batch);
          return batch;
        }));
  }

  void prepareEphemeralStorageExpects() {
    ON_CALL(*trie_storage_, getEphemeralBatchAt(_))
        .WillByDefault(testing::Invoke([this](auto &root) {
          auto batch = std::make_unique<TrieBatchMock>();
          prepareStorageBatchExpectations(*batch);
          return batch;
        }));
  }

  template <typename BatchMock>
  void prepareStorageBatchExpectations(BatchMock &batch) {
    ON_CALL(batch, tryGetMock(_)).WillByDefault(testing::Return(std::nullopt));
    ON_CALL(batch, put(_, _))
        .WillByDefault(testing::Return(outcome::success()));
    ON_CALL(batch, remove(_))
        .WillByDefault(testing::Return(outcome::success()));
    ON_CALL(batch, clearPrefix(_, _))
        .WillByDefault(testing::Return(outcome::success()));
    ON_CALL(batch, trieCursor()).WillByDefault(testing::Invoke([] {
      auto cursor = std::make_unique<storage::trie::PolkadotTrieCursorMock>();
      ON_CALL(*cursor, seekUpperBound(_))
          .WillByDefault(Return(outcome::success()));
      return cursor;
    }));
    static auto heappages_key = ":heappages"_buf;
    EXPECT_CALL(batch, tryGetMock(heappages_key.view()))
        .Times(testing::AnyNumber());
  }

  primitives::BlockHeader createBlockHeader(const primitives::BlockHash &hash,
                                            primitives::BlockNumber number) {
    common::Hash256 parent_hash = "genesis_hash"_hash256;

    common::Hash256 state_root = "state_root"_hash256;

    common::Hash256 extrinsics_root = "extrinsics_root"_hash256;

    Digest digest{};

    ON_CALL(*block_tree_, getHashByNumber(number))
        .WillByDefault(testing::Return(hash));
    ON_CALL(*block_tree_, getNumberByHash(hash))
        .WillByDefault(testing::Return(number));

    BlockHeader header{
        number, parent_hash, state_root, extrinsics_root, digest};
    EXPECT_CALL(*block_tree_, getBlockHeader(hash))
        .WillRepeatedly(testing::Return(header));
    return header;
  }

  primitives::Block createBlock(const primitives::BlockHash &hash,
                                primitives::BlockNumber number) {
    auto header = createBlockHeader(hash, number);

    std::vector<Extrinsic> xts;

    return Block{header, xts};
  }

 protected:
  AppConfigurationMock app_config_;
  std::shared_ptr<testing::NiceMock<blockchain::BlockTreeMock>> block_tree_;
  std::shared_ptr<runtime::RuntimeCodeProvider> wasm_provider_;
  std::shared_ptr<storage::trie::TrieStorageMock> trie_storage_;
  std::shared_ptr<storage::trie::TrieSerializerMock> serializer_;
  std::shared_ptr<runtime::RuntimePropertiesCacheMock> cache_;
  std::shared_ptr<runtime::Executor> executor_;
  std::shared_ptr<runtime::RuntimeContextFactoryImpl> ctx_factory_;
  std::shared_ptr<offchain::OffchainPersistentStorageMock> offchain_storage_;
  std::shared_ptr<offchain::OffchainWorkerPoolMock> offchain_worker_pool_;
  std::shared_ptr<crypto::HasherImpl> hasher_;
  std::shared_ptr<host_api::HostApiFactory> host_api_factory_;
  std::shared_ptr<RuntimeInstancesPoolImpl> instance_pool_;
};

class RuntimeTestBase : public ::testing::Test, public RuntimeTestBaseImpl {
 public:
  void SetUp() override {
    SetUpImpl();
  }
};
