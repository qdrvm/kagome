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

  void initStorage() {
    using kagome::storage::trie::EphemeralTrieBatchMock;
    using kagome::storage::trie::PersistentTrieBatch;
    using kagome::storage::trie::PersistentTrieBatchMock;
    using kagome::storage::trie::TopperTrieBatchMock;

    batch_mock_ = std::make_shared<PersistentTrieBatchMock>();
    ON_CALL(*batch_mock_, get(_)).WillByDefault(testing::Invoke([](auto &key) {
      return kagome::common::Buffer();
    }));
    ON_CALL(*batch_mock_, put(_, _))
        .WillByDefault(testing::Return(outcome::success()));
    ON_CALL(*batch_mock_, remove(_))
        .WillByDefault(testing::Return(outcome::success()));
    ON_CALL(*batch_mock_, clearPrefix(_, _))
        .WillByDefault(testing::Return(outcome::success()));
    ON_CALL(*batch_mock_, batchOnTop()).WillByDefault(testing::Invoke([] {
      return std::make_unique<TopperTrieBatchMock>();
    }));
    ON_CALL(*batch_mock_, trieCursor()).WillByDefault(testing::Invoke([] {
      auto cursor =
          std::make_unique<kagome::storage::trie::PolkadotTrieCursorMock>();
      ON_CALL(*cursor, seekUpperBound(_))
          .WillByDefault(Return(outcome::success()));
      return cursor;
    }));
    storage_provider_ =
        std::make_shared<kagome::runtime::TrieStorageProviderMock>();
    ON_CALL(*storage_provider_, getCurrentBatch())
        .WillByDefault(testing::Return(batch_mock_));
    ON_CALL(*storage_provider_, tryGetPersistentBatch())
        .WillByDefault(testing::Invoke(
            [&]() -> boost::optional<std::shared_ptr<PersistentTrieBatch>> {
              return std::shared_ptr<PersistentTrieBatch>(batch_mock_);
            }));
    ON_CALL(*storage_provider_, setToPersistent())
        .WillByDefault(testing::Return(outcome::success()));
    ON_CALL(*storage_provider_, setToPersistentAt("genesis state root"_hash256))
        .WillByDefault(testing::Return(outcome::success()));
    ON_CALL(*storage_provider_, setToEphemeral())
        .WillByDefault(testing::Return(outcome::success()));
    ON_CALL(*storage_provider_, setToEphemeralAt("genesis state root"_hash256))
        .WillByDefault(testing::Return(outcome::success()));
    ON_CALL(*storage_provider_, rollbackTransaction())
        .WillByDefault(testing::Return(
            outcome::failure(kagome::runtime::RuntimeTransactionError::
                                 NO_TRANSACTIONS_WERE_STARTED)));
    ON_CALL(*storage_provider_, getLatestRootMock())
        .WillByDefault(testing::Return("42"_hash256));

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

    header_repo_ =
        std::make_shared<kagome::blockchain::BlockHeaderRepositoryMock>();

    EXPECT_CALL(*header_repo_, getHashByNumber(0))
        .WillRepeatedly(testing::Return("genesis hash"_hash256));
    EXPECT_CALL(*header_repo_,
                getBlockHeader(testing::AnyOf(
                    kagome::primitives::BlockId{0},
                    kagome::primitives::BlockId{"genesis hash"_hash256})))
        .WillRepeatedly(testing::Return(kagome::primitives::BlockHeader{
            .parent_hash = {},
            .number = 0,
            .state_root = {"genesis state root"_hash256},
            .extrinsics_root = {"genesis ext root"_hash256},
            .digest = {}}));
  }

  struct ImplementationSpecificRuntimeClasses {
    std::shared_ptr<kagome::runtime::ModuleFactory> module_factory_;
    std::shared_ptr<kagome::runtime::CoreApiFactory> core_api_factory_;
    std::shared_ptr<kagome::runtime::MemoryProvider> memory_provider_;
    std::shared_ptr<kagome::host_api::HostApi> host_api_;
  };

  virtual ImplementationSpecificRuntimeClasses getImplementationSpecific() = 0;

  void SetUp() override {
    initStorage();

    auto &&[module_factory, core_api_factory, memory_provider, host_api] =
        getImplementationSpecific();

    auto wasm_path = boost::filesystem::path(__FILE__).parent_path().string()
                     + "/wasm/sub2dev.wasm";
    wasm_provider_ =
        std::make_shared<kagome::runtime::BasicCodeProvider>(wasm_path);

    auto trie_storage =
        std::make_shared<kagome::storage::trie::TrieStorageMock>();
    auto upgrade_tracker =
        std::make_shared<kagome::runtime::RuntimeUpgradeTrackerImpl>(
            header_repo_,
            std::make_shared<kagome::storage::InMemoryStorage>(),
            kagome::application::CodeSubstitutes{});

    auto module_repo = std::make_shared<kagome::runtime::ModuleRepositoryImpl>(
        upgrade_tracker, module_factory);

    runtime_env_factory_ =
        std::make_shared<kagome::runtime::RuntimeEnvironmentFactory>(
            std::move(wasm_provider_),
            std::move(module_repo),
            header_repo_);

    executor_ = std::make_shared<kagome::runtime::Executor>(
        header_repo_, runtime_env_factory_);
  }

  void preparePersistentStorageExpects() {
    EXPECT_CALL(*storage_provider_, setToPersistentAt(_));
    EXPECT_CALL(*storage_provider_, tryGetPersistentBatch());
    prepareCommonStorageExpects();
  }

  void prepareEphemeralStorageExpects() {
    EXPECT_CALL(*storage_provider_, setToEphemeralAt(_));
    prepareCommonStorageExpects();
  }

  kagome::primitives::BlockHeader createBlockHeader() {
    kagome::common::Hash256 parent_hash = "genesis hash"_hash256;

    kagome::primitives::BlockNumber number = 1;

    kagome::common::Hash256 state_root;
    state_root.fill('s');

    kagome::common::Hash256 extrinsics_root;
    extrinsics_root.fill('e');

    Digest digest{};

    // runtime checks the correctness of the parent hash
    auto parent_number_enc = kagome::scale::encode(number - 1).value();
    ON_CALL(
        *batch_mock_,
        get(Buffer{hasher_->twox_64(parent_number_enc)}.putUint32(number - 1)))
        .WillByDefault(testing::Invoke([parent_hash](auto &) {
          std::cerr << "Get the parent root";
          return Buffer{parent_hash};
        }));
    ON_CALL(*header_repo_, getHashByNumber(number))
        .WillByDefault(testing::Return(parent_hash));
    ON_CALL(*header_repo_, getNumberByHash(parent_hash))
        .WillByDefault(testing::Return(number));

    return BlockHeader{
        parent_hash, number, state_root, extrinsics_root, digest};
  }

  kagome::primitives::Block createBlock() {
    auto header = createBlockHeader();

    std::vector<Extrinsic> xts;

    return Block{header, xts};
  }

 private:
  void prepareCommonStorageExpects() {
    auto heappages_key_res =
        kagome::common::Buffer::fromString(std::string(":heappages"));
    if (not heappages_key_res.has_value()) {
      GTEST_FAIL() << heappages_key_res.error().message();
    }
    auto &&heappages_key = heappages_key_res.value();
    EXPECT_CALL(*storage_provider_, getCurrentBatch());
    EXPECT_CALL(*batch_mock_, get(heappages_key));
  }

 protected:
  std::shared_ptr<kagome::blockchain::BlockHeaderRepositoryMock> header_repo_;
  std::shared_ptr<kagome::runtime::TrieStorageProviderMock> storage_provider_;
  std::shared_ptr<kagome::storage::trie::PersistentTrieBatchMock> batch_mock_;
  std::shared_ptr<kagome::runtime::RuntimeCodeProvider> wasm_provider_;
  std::shared_ptr<kagome::runtime::RuntimeEnvironmentFactory>
      runtime_env_factory_;
  std::shared_ptr<kagome::runtime::Executor> executor_;
  std::shared_ptr<kagome::storage::changes_trie::ChangesTrackerMock>
      changes_tracker_;
  std::shared_ptr<kagome::crypto::Hasher> hasher_;
  std::shared_ptr<kagome::host_api::HostApiFactory> host_api_factory_;
};

#endif  // KAGOME_TEST_CORE_RUNTIME_RUNTIME_TEST_BASE_HPP
