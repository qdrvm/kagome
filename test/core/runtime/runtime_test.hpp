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

#include "crypto/hasher/hasher_impl.hpp"
#include "extensions/impl/extension_factory_impl.hpp"
#include "mock/core/runtime/trie_storage_provider_mock.hpp"
#include "mock/core/storage/changes_trie/changes_tracker_mock.hpp"
#include "mock/core/storage/trie/trie_batches_mock.hpp"
#include "mock/core/storage/trie/trie_storage_mock.hpp"
#include "primitives/block.hpp"
#include "primitives/block_header.hpp"
#include "primitives/block_id.hpp"
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
    using kagome::storage::trie::PersistentTrieBatchMock;

    auto storage_provider = std::make_shared<
        testing::NiceMock<kagome::runtime::TrieStorageProviderMock>>();
    ON_CALL(*storage_provider, getCurrentBatch())
        .WillByDefault(testing::Invoke(
            []() { return std::make_unique<PersistentTrieBatchMock>(); }));
    ON_CALL(*storage_provider, setToPersistent())
        .WillByDefault(testing::Return(outcome::success()));
    ON_CALL(*storage_provider, setToEphemeral())
        .WillByDefault(testing::Return(outcome::success()));

    changes_tracker_ =
        std::make_shared<kagome::storage::changes_trie::ChangesTrackerMock>();
    auto extension_factory =
        std::make_shared<kagome::extensions::ExtensionFactoryImpl>(
            changes_tracker_);
    auto wasm_path = boost::filesystem::path(__FILE__).parent_path().string()
                     + "/wasm/polkadot_runtime.compact.wasm";
    auto wasm_provider =
        std::make_shared<kagome::runtime::BasicWasmProvider>(wasm_path);
    auto hasher = std::make_shared<kagome::crypto::HasherImpl>();

    runtime_manager_ =
        std::make_shared<kagome::runtime::binaryen::RuntimeManager>(
            std::move(wasm_provider),
            std::move(extension_factory),
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

    Digest digest;

    return BlockHeader{
        parent_hash, number, state_root, extrinsics_root, {digest}};
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
  std::shared_ptr<kagome::runtime::binaryen::RuntimeManager> runtime_manager_;
  std::shared_ptr<kagome::storage::changes_trie::ChangesTracker>
      changes_tracker_;
};

#endif  // KAGOME_RUNTIME_TEST_HPP
