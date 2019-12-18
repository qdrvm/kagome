/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_RUNTIME_TEST_HPP
#define KAGOME_RUNTIME_TEST_HPP

#include <fstream>
#include <memory>

#include <gtest/gtest.h>
#include <boost/filesystem/path.hpp>
#include "core/storage/trie/mock_trie_db.hpp"
#include "extensions/extension_impl.hpp"
#include "primitives/block.hpp"
#include "primitives/block_header.hpp"
#include "primitives/block_id.hpp"
#include "runtime/impl/wasm_memory_impl.hpp"
#include "testutil/outcome.hpp"
#include "runtime/impl/basic_wasm_provider.hpp"

class RuntimeTest : public ::testing::Test {
 public:
  using Buffer = kagome::common::Buffer;
  using Block = kagome::primitives::Block;
  using BlockId = kagome::primitives::BlockId;
  using BlockHeader = kagome::primitives::BlockHeader;
  using Extrinsic = kagome::primitives::Extrinsic;
  using Digest = kagome::primitives::Digest;

  void SetUp() override {
    trie_db_ = std::make_shared<kagome::storage::trie::MockTrieDb>();
    memory_ = std::make_shared<kagome::runtime::WasmMemoryImpl>();
    extension_ =
        std::make_shared<kagome::extensions::ExtensionImpl>(memory_, trie_db_);
    std::string wasm_path =
        boost::filesystem::path(__FILE__).parent_path().string()
        + "/wasm/polkadot_runtime.compact.wasm";
    wasm_provider_ = std::make_shared<test::BasicWasmProvider>(wasm_path);
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
  std::shared_ptr<kagome::storage::trie::MockTrieDb> trie_db_;
  std::shared_ptr<kagome::runtime::WasmMemory> memory_;
  std::shared_ptr<kagome::extensions::ExtensionImpl> extension_;
  std::shared_ptr<test::BasicWasmProvider> wasm_provider_;
};

#endif  // KAGOME_RUNTIME_TEST_HPP
