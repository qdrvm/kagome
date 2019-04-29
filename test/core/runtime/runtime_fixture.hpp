/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_RUNTIME_FIXTURE_HPP
#define KAGOME_RUNTIME_FIXTURE_HPP

#include <fstream>
#include <memory>

#include <gtest/gtest.h>
#include <boost/filesystem/path.hpp>
#include "primitives/block.hpp"
#include "primitives/block_header.hpp"
#include "core/storage/merkle/mock_trie_db.hpp"
#include "extensions/extension_impl.hpp"
#include "primitives/impl/scale_codec_impl.hpp"
#include "runtime/impl/wasm_memory_impl.hpp"
#include "testutil/outcome.hpp"

using kagome::common::Buffer;
using kagome::extensions::ExtensionImpl;
using kagome::primitives::Block;
using kagome::primitives::BlockHeader;
using kagome::primitives::BlockId;
using kagome::primitives::BlockNumber;
using kagome::primitives::Extrinsic;
using kagome::primitives::ScaleCodecImpl;
using kagome::runtime::WasmMemory;
using kagome::runtime::WasmMemoryImpl;
using kagome::storage::merkle::MockTrieDb;

class RuntimeTestFixture : public ::testing::Test {
 public:
  void SetUp() override {
    trie_db_ = std::make_shared<MockTrieDb>();
    memory_ = std::make_shared<WasmMemoryImpl>();
    extension_ = std::make_shared<ExtensionImpl>(memory_, trie_db_);
    codec_ = std::make_shared<ScaleCodecImpl>();
  }

  Buffer getRuntimeCode() {
    // get file from wasm/ folder
    auto path =  boost::filesystem::path(__FILE__).parent_path().string()
                + "/wasm/polkadot_runtime.compact.wasm";
    std::ifstream ifd(path, std::ios::binary | std::ios::ate);
    int size = ifd.tellg();
    ifd.seekg(0, std::ios::beg);
    Buffer b(size, 0);
    ifd.read((char *)b.toBytes(), size);
    return b;
  }

  BlockHeader createBlockHeader() {
    kagome::common::Hash256 parent_hash{};
    parent_hash.fill('p');

    size_t number = 1;

    kagome::common::Hash256 state_root{};
    state_root.fill('s');

    kagome::common::Hash256 extrinsics_root{};
    state_root.fill('e');

    Buffer digest{};

    BlockHeader header(parent_hash, number, state_root,
                       extrinsics_root, digest);
    return header;
  }

  Block createBlock() {
    auto header = createBlockHeader();

    std::vector<Extrinsic> xts;

    xts.emplace_back(Buffer{'a', 'b', 'c'});
    xts.emplace_back(Buffer{'1', '2', '3'});

    Block b(header, xts);

    return b;
  }

  BlockId createBlockId() {
    BlockId res = BlockNumber{0};
    return res;
  }

 protected:
  std::shared_ptr<MockTrieDb> trie_db_;
  std::shared_ptr<WasmMemory> memory_;
  std::shared_ptr<ExtensionImpl> extension_;
  std::shared_ptr<ScaleCodecImpl> codec_;
};

#endif  // KAGOME_RUNTIME_FIXTURE_HPP
