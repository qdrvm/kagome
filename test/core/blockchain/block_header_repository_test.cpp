/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "blockchain/block_header_repository.hpp"

#include <gtest/gtest.h>
#include "blockchain/impl/level_db_block_header_repository.hpp"
#include "blockchain/impl/level_db_util.hpp"
#include "crypto/hasher/hasher_impl.hpp"
#include "scale/scale.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"
#include "testutil/storage/base_leveldb_test.hpp"
#include <iostream>

using kagome::blockchain::LevelDbBlockHeaderRepository;
using kagome::blockchain::numberAndHashToLookupKey;
using kagome::blockchain::numberToIndexKey;
using kagome::blockchain::prependPrefix;
using kagome::blockchain::prefix::Prefix;
using kagome::common::Buffer;
using kagome::primitives::BlockHeader;
using kagome::primitives::BlockId;
using kagome::primitives::BlockNumber;

class BlockHeaderRepository_Test : public test::BaseLevelDB_Test {
 public:
  BlockHeaderRepository_Test()
      : BaseLevelDB_Test(fs::path("/tmp/blockheaderrepotest.lvldb")) {}

  void SetUp() override {
    open();
  }
};

TEST_F(BlockHeaderRepository_Test, Create) {
  auto hasher = std::make_shared<kagome::hash::HasherImpl>();
  auto header_repo =
      std::make_shared<LevelDbBlockHeaderRepository>(*db_, hasher);

  BlockHeader header;
  header.number = 42;
  Buffer enc_header{kagome::scale::encode(header).value()};
  auto hash = hasher->blake2_256(enc_header);

  auto lookup_key = numberAndHashToLookupKey(42, hash);

  EXPECT_OUTCOME_TRUE_1(
      db_->put(prependPrefix(lookup_key, Prefix::HEADER), enc_header));
  EXPECT_OUTCOME_TRUE_1(
      db_->put(prependPrefix(numberToIndexKey(42), Prefix::ID_TO_LOOKUP_KEY),
               lookup_key));
  std::cout << kagome::common::hex_lower(prependPrefix(numberToIndexKey(42),  Prefix::ID_TO_LOOKUP_KEY)) << "\n";
  EXPECT_OUTCOME_TRUE_1(db_->put(
      prependPrefix(Buffer{hash}, Prefix::ID_TO_LOOKUP_KEY), lookup_key));
  std::cout << kagome::common::hex_lower(prependPrefix(Buffer{hash}, Prefix::ID_TO_LOOKUP_KEY)) << "\n";

  ASSERT_TRUE(std::equal(hash.begin(), hash.end(), header_repo->getHashByNumber(42).value().value().begin()));
  ASSERT_EQ(header_repo->getNumberByHash(hash).value().value(), 42);
}
