/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TEST_CORE_RUNTIME_RUNTIME_TEST_BASE_HPP
#define KAGOME_TEST_CORE_RUNTIME_RUNTIME_TEST_BASE_HPP

#include <gtest/gtest.h>

#include <boost/filesystem/path.hpp>
#include <fstream>
#include <memory>

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
#include "mock/core/runtime/trie_storage_provider_mock.hpp"
#include "mock/core/storage/changes_trie/changes_tracker_mock.hpp"
#include "mock/core/storage/trie/polkadot_trie_cursor_mock.h"
#include "mock/core/storage/trie/trie_batches_mock.hpp"
#include "mock/core/storage/trie/trie_storage_mock.hpp"
#include "primitives/block.hpp"
#include "primitives/block_header.hpp"
#include "primitives/block_id.hpp"
#include "runtime/common/module_repository_impl.hpp"
#include "runtime/common/runtime_transaction_error.hpp"
#include "runtime/common/runtime_upgrade_tracker_impl.hpp"
#include "runtime/core_api_factory.hpp"
#include "runtime/executor.hpp"
#include "runtime/runtime_environment_factory.hpp"
#include "storage/in_memory/in_memory_storage.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"
#include "testutil/runtime/common/basic_code_provider.hpp"

using testing::_;
using testing::Return;

class RuntimeTestBase : public ::testing::Test {
 public:
  using Buffer = kagome::common::Buffer;
  using Block = kagome::primitives::Block;
  using BlockId = kagome::primitives::BlockId;
  using BlockHeader = kagome::primitives::BlockHeader;
  using Extrinsic = kagome::primitives::Extrinsic;
  using Digest = kagome::primitives::Digest;
  using PersistentTrieBatchMock =
      kagome::storage::trie::PersistentTrieBatchMock;
  using EphemeralTrieBatchMock = kagome::storage::trie::EphemeralTrieBatchMock;
  using TopperTrieBatchMock = kagome::storage::trie::TopperTrieBatchMock;

  void initStorage() {
    using kagome::storage::trie::PersistentTrieBatch;
    using kagome::storage::trie::TopperTrieBatchMock;

    auto random_generator =
        std::make_shared<kagome::crypto::BoostRandomGenerator>();
    auto sr25519_provider =
        std::make_shared<kagome::crypto::Sr25519ProviderImpl>(random_generator);
    auto ed25519_provider =
        std::make_shared<kagome::crypto::Ed25519ProviderImpl>(random_generator);
    auto secp256k1_provider =
        std::make_shared<kagome::crypto::Secp256k1ProviderImpl>();
    hasher_ = std::make_shared<kagome::crypto::HasherImpl>();
    auto pbkdf2_provider =
        std::make_shared<kagome::crypto::Pbkdf2ProviderImpl>();
    auto bip39_provider =
        std::make_shared<kagome::crypto::Bip39ProviderImpl>(pbkdf2_provider);
    auto keystore_path =
        boost::filesystem::temp_directory_path()
        / boost::filesystem::unique_path("kagome_keystore_test_dir");
    auto crypto_store = std::make_shared<kagome::crypto::CryptoStoreImpl>(
        std::make_shared<kagome::crypto::Ed25519Suite>(ed25519_provider),
        std::make_shared<kagome::crypto::Sr25519Suite>(sr25519_provider),
        bip39_provider,
        kagome::crypto::KeyFileStorage::createAt(keystore_path).value());
    changes_tracker_ =
        std::make_shared<kagome::storage::changes_trie::ChangesTrackerMock>();

    host_api_factory_ = std::make_shared<kagome::host_api::HostApiFactoryImpl>(
        changes_tracker_,
        sr25519_provider,
        ed25519_provider,
        secp256k1_provider,
        hasher_,
        crypto_store,
        bip39_provider);

    header_repo_ = std::make_shared<
        testing::NiceMock<kagome::blockchain::BlockHeaderRepositoryMock>>();

    ON_CALL(*header_repo_, getHashByNumber(0))
        .WillByDefault(testing::Return("genesis_hash"_hash256));
    EXPECT_CALL(*header_repo_,
            getBlockHeader(testing::AnyOf(
                kagome::primitives::BlockId{0},
                kagome::primitives::BlockId{"genesis_hash"_hash256})))
        .WillRepeatedly(testing::Return(kagome::primitives::BlockHeader{
            .parent_hash = {},
            .number = 0,
            .state_root = {"genesis state root"_hash256},
            .extrinsics_root = {"genesis ext root"_hash256},
            .digest = {}}));
  }

  virtual std::shared_ptr<kagome::runtime::ModuleFactory>
  createModuleFactory() = 0;

  void SetUp() override {
    initStorage();
    trie_storage_ = std::make_shared<kagome::storage::trie::TrieStorageMock>();

    auto module_factory = createModuleFactory();

    auto wasm_path = boost::filesystem::path(__FILE__).parent_path().string()
                     + "/wasm/sub2dev.wasm";
    wasm_provider_ =
        std::make_shared<kagome::runtime::BasicCodeProvider>(wasm_path);

    auto upgrade_tracker =
        std::make_shared<kagome::runtime::RuntimeUpgradeTrackerImpl>(
            header_repo_,
            std::make_shared<kagome::storage::InMemoryStorage>(),
            kagome::application::CodeSubstitutes{});

    auto module_repo = std::make_shared<kagome::runtime::ModuleRepositoryImpl>(
        upgrade_tracker, module_factory);

    runtime_env_factory_ =
        std::make_shared<kagome::runtime::RuntimeEnvironmentFactory>(
            std::move(wasm_provider_), std::move(module_repo), header_repo_);

    executor_ = std::make_shared<kagome::runtime::Executor>(
        header_repo_, runtime_env_factory_);
  }

  void preparePersistentStorageExpects() {
    EXPECT_CALL(*trie_storage_, getPersistentBatchAt(_))
        .WillOnce(testing::Invoke([this](auto &root) {
          auto batch = std::make_unique<PersistentTrieBatchMock>();
          prepareStorageBatchExpectations(*batch);
          return batch;
        }));
  }

  void prepareEphemeralStorageExpects() {
    EXPECT_CALL(*trie_storage_, getEphemeralBatchAt(_))
        .WillOnce(testing::Invoke([this](auto &root) {
          auto batch = std::make_unique<EphemeralTrieBatchMock>();
          prepareStorageBatchExpectations(*batch);
          return batch;
        }));
  }

  template <typename BatchMock>
  void prepareStorageBatchExpectations(BatchMock &batch) {
    ON_CALL(batch, get(_)).WillByDefault(testing::Invoke([](auto &key) {
      return kagome::common::Buffer();
    }));
    ON_CALL(batch, put(_, _))
        .WillByDefault(testing::Return(outcome::success()));
    ON_CALL(batch, remove(_))
        .WillByDefault(testing::Return(outcome::success()));
    ON_CALL(batch, clearPrefix(_, _))
        .WillByDefault(testing::Return(outcome::success()));
    ON_CALL(batch, trieCursor()).WillByDefault(testing::Invoke([] {
      auto cursor =
          std::make_unique<kagome::storage::trie::PolkadotTrieCursorMock>();
      ON_CALL(*cursor, seekUpperBound(_))
          .WillByDefault(Return(outcome::success()));
      return cursor;
    }));
    auto heappages_key = ":heappages"_buf;
    EXPECT_CALL(batch, get(heappages_key));
  }

  kagome::primitives::BlockHeader createBlockHeader(
      kagome::primitives::BlockHash const &hash,
      kagome::primitives::BlockNumber number) {
    kagome::common::Hash256 parent_hash = "genesis_hash"_hash256;

    kagome::common::Hash256 state_root = "state_root"_hash256;

    kagome::common::Hash256 extrinsics_root = "extrinsics_root"_hash256;

    Digest digest{};

    ON_CALL(*header_repo_, getHashByNumber(number))
        .WillByDefault(testing::Return(hash));
    ON_CALL(*header_repo_, getNumberByHash(hash))
        .WillByDefault(testing::Return(number));

    BlockHeader header{
        parent_hash, number, state_root, extrinsics_root, digest};
    EXPECT_CALL(*header_repo_, getBlockHeader(testing::AnyOf(hash, number)))
        .WillRepeatedly(testing::Return(header));
    return header;
  }

  kagome::primitives::Block createBlock(
      kagome::primitives::BlockHash const &hash,
      kagome::primitives::BlockNumber number) {
    auto header = createBlockHeader(hash, number);

    std::vector<Extrinsic> xts;

    return Block{header, xts};
  }

 protected:
  std::shared_ptr<
      testing::NiceMock<kagome::blockchain::BlockHeaderRepositoryMock>>
      header_repo_;
  std::shared_ptr<kagome::runtime::RuntimeCodeProvider> wasm_provider_;
  std::shared_ptr<kagome::storage::trie::TrieStorageMock> trie_storage_;
  std::shared_ptr<kagome::runtime::RuntimeEnvironmentFactory>
      runtime_env_factory_;
  std::shared_ptr<kagome::runtime::Executor> executor_;
  std::shared_ptr<kagome::storage::changes_trie::ChangesTrackerMock>
      changes_tracker_;
  std::shared_ptr<kagome::crypto::Hasher> hasher_;
  std::shared_ptr<kagome::host_api::HostApiFactory> host_api_factory_;
};

#endif  // KAGOME_TEST_CORE_RUNTIME_RUNTIME_TEST_BASE_HPP
