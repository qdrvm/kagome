/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/impl/core_impl.hpp"

#include <fstream>

#include <gtest/gtest.h>
#include <boost/filesystem.hpp>
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
using kagome::runtime::CoreImpl;
using kagome::runtime::WasmMemory;
using kagome::runtime::WasmMemoryImpl;
using kagome::storage::merkle::MockTrieDb;

using ::testing::_;
using ::testing::Return;

namespace fs = boost::filesystem;

class CoreTest : public ::testing::Test {
 public:
  void SetUp() override {
    trie_db_ = std::make_shared<MockTrieDb>();
    memory_ = std::make_shared<WasmMemoryImpl>();
    extension_ = std::make_shared<ExtensionImpl>(memory_, trie_db_);
    codec_ = std::make_shared<ScaleCodecImpl>();

    auto state_code = getRuntimeCode();
    core_ = std::make_shared<CoreImpl>(state_code, extension_, codec_);
  }

  Buffer getRuntimeCode() {
    // get file from wasm/ folder
    auto path = fs::path(__FILE__).parent_path().string()
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

    BlockHeader header(Buffer{parent_hash}, number, Buffer{state_root},
                       Buffer{extrinsics_root}, digest);
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
  std::shared_ptr<CoreImpl> core_;
};

/**
 * @given initialized core api
 * @when version is invoked
 * @then successful result is returned
 */
TEST_F(CoreTest, VersionTest) {
  ASSERT_TRUE(core_->version());
}

/**
 * @given initialized core api
 * @when execute_block is invoked
 * @then successful result is returned
 */
TEST_F(CoreTest, DISABLED_ExecuteBlockTest) {
  auto block = createBlock();

  ASSERT_TRUE(core_->execute_block(block));
}

/**
 * @given initialised core api
 * @when initialise_block is invoked
 * @then successful result is returned
 */
TEST_F(CoreTest, DISABLED_InitializeBlockTest) {
  auto header = createBlockHeader();

  ASSERT_TRUE(core_->initialise_block(header));
}

/**
 * @given initialized core api
 * @when authorities is invoked
 * @then successful result is returned
 */
TEST_F(CoreTest, DISABLED_AuthoritiesTest) {
  auto block_id = createBlockId();

  ASSERT_TRUE(core_->authorities(block_id));
}
