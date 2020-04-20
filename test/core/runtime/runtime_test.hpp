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

#include "mock/core/storage/trie/trie_db_mock.hpp"
#include "extensions/impl/extension_factory_impl.hpp"
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
	  auto trie_db = std::make_shared<kagome::storage::trie::TrieDbMock>();
    auto extension_factory =
        std::make_shared<kagome::extensions::ExtensionFactoryImpl>(trie_db);
    auto wasm_path =
        boost::filesystem::path(__FILE__).parent_path().string()
        + "/wasm/polkadot_runtime.compact.wasm";
    auto wasm_provider =
        std::make_shared<kagome::runtime::BasicWasmProvider>(wasm_path);
    runtime_manager_ =
        std::make_shared<kagome::runtime::binaryen::RuntimeManager>(
            wasm_provider, extension_factory);
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
};

#endif  // KAGOME_RUNTIME_TEST_HPP
