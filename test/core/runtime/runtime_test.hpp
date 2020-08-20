/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_RUNTIME_TEST_HPP
#define KAGOME_RUNTIME_TEST_HPP

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
#include "extensions/impl/extension_factory_impl.hpp"
#include "mock/core/blockchain/block_header_repository_mock.hpp"
#include "mock/core/runtime/trie_storage_provider_mock.hpp"
#include "mock/core/storage/changes_trie/changes_tracker_mock.hpp"
#include "mock/core/storage/trie/trie_batches_mock.hpp"
#include "mock/core/storage/trie/trie_storage_mock.hpp"
#include "primitives/block.hpp"
#include "primitives/block_header.hpp"
#include "primitives/block_id.hpp"
#include "runtime/binaryen/module/wasm_module_factory_impl.hpp"
#include "runtime/binaryen/runtime_api/core_factory_impl.hpp"
#include "runtime/binaryen/runtime_manager.hpp"
#include "runtime/binaryen/wasm_memory_impl.hpp"
#include "testutil/outcome.hpp"
#include "testutil/runtime/common/basic_wasm_provider.hpp"

class RuntimeTest : public ::testing::Test {
 public:
  using Buffer = kagome::common::Buffer;
  using Block = kagome::primitives::Block;
  using BlockId = kagome::primitives::BlockId;
  using BlockHeader = kagome::primitives::BlockHeader;
  using Extrinsic = kagome::primitives::Extrinsic;
  using Digest = kagome::primitives::Digest;

  void SetUp() override {
    using kagome::storage::trie::EphemeralTrieBatchMock;
    using kagome::storage::trie::PersistentTrieBatch;
    using kagome::storage::trie::PersistentTrieBatchMock;

    auto storage_provider =
        std::make_shared<kagome::runtime::TrieStorageProviderMock>();
    ON_CALL(*storage_provider, getCurrentBatch())
        .WillByDefault(testing::Invoke(
            []() { return std::make_unique<PersistentTrieBatchMock>(); }));
    ON_CALL(*storage_provider, tryGetPersistentBatch())
        .WillByDefault(testing::Invoke(
            []() -> boost::optional<std::shared_ptr<PersistentTrieBatch>> {
              return std::shared_ptr<PersistentTrieBatch>(
                  std::make_shared<PersistentTrieBatchMock>());
            }));
    ON_CALL(*storage_provider, setToPersistent())
        .WillByDefault(testing::Return(outcome::success()));
    ON_CALL(*storage_provider, setToEphemeral())
        .WillByDefault(testing::Return(outcome::success()));

    auto random_generator =
        std::make_shared<kagome::crypto::BoostRandomGenerator>();
    auto sr25519_provider =
        std::make_shared<kagome::crypto::SR25519ProviderImpl>(random_generator);
    auto ed25519_provider =
        std::make_shared<kagome::crypto::ED25519ProviderImpl>();
    auto secp256k1_provider =
        std::make_shared<kagome::crypto::Secp256k1ProviderImpl>();
    auto hasher = std::make_shared<kagome::crypto::HasherImpl>();
    auto pbkdf2_provider =
        std::make_shared<kagome::crypto::Pbkdf2ProviderImpl>();
    auto bip39_provider =
        std::make_shared<kagome::crypto::Bip39ProviderImpl>(pbkdf2_provider);
    auto crypto_store =
        std::make_shared<kagome::crypto::CryptoStoreImpl>(ed25519_provider,
                                                          sr25519_provider,
                                                          secp256k1_provider,
                                                          bip39_provider,
                                                          random_generator);
    changes_tracker_ =
        std::make_shared<kagome::storage::changes_trie::ChangesTrackerMock>();

    auto extension_factory =
        std::make_shared<kagome::extensions::ExtensionFactoryImpl>(
            changes_tracker_,
            sr25519_provider,
            ed25519_provider,
            secp256k1_provider,
            hasher,
            crypto_store,
            bip39_provider,
            [this](
                std::shared_ptr<kagome::runtime::WasmProvider> wasm_provider) {
              kagome::runtime::binaryen::CoreFactoryImpl factory(
                  runtime_manager_,
                  changes_tracker_,
                  std::make_shared<
                      kagome::blockchain::BlockHeaderRepositoryMock>());
              return factory.createWithCode(std::move(wasm_provider));
            });

    auto module_factory =
        std::make_shared<kagome::runtime::binaryen::WasmModuleFactoryImpl>();

    auto wasm_path = boost::filesystem::path(__FILE__).parent_path().string()
                     + "/wasm/polkadot_runtime.compact.wasm";
    wasm_provider_ =
        std::make_shared<kagome::runtime::BasicWasmProvider>(wasm_path);

    runtime_manager_ =
        std::make_shared<kagome::runtime::binaryen::RuntimeManager>(
            std::move(extension_factory),
            std::move(module_factory),
            std::move(storage_provider),
            std::move(hasher));
  }

  kagome::primitives::BlockHeader createBlockHeader() {
    kagome::common::Hash256 parent_hash;
    parent_hash.fill('p');

    size_t number = 1;

    kagome::common::Hash256 state_root;
    state_root.fill('s');

    kagome::common::Hash256 extrinsics_root;
    extrinsics_root.fill('e');

    Digest digest{};

    return BlockHeader{
        parent_hash, number, state_root, extrinsics_root, digest};
  }

  kagome::primitives::Block createBlock() {
    auto header = createBlockHeader();

    std::vector<Extrinsic> xts;

    xts.emplace_back(Extrinsic{Buffer{'a', 'b', 'c'}});
    xts.emplace_back(Extrinsic{Buffer{'1', '2', '3'}});

    return Block{header, xts};
  }

  kagome::primitives::BlockId createBlockId() {
    BlockId res = kagome::primitives::BlockNumber{0};
    return res;
  }

 protected:
  std::shared_ptr<kagome::runtime::WasmProvider> wasm_provider_;
  std::shared_ptr<kagome::runtime::binaryen::RuntimeManager> runtime_manager_;
  std::shared_ptr<kagome::storage::changes_trie::ChangesTracker>
      changes_tracker_;
};

#endif  // KAGOME_RUNTIME_TEST_HPP
