/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/grandpa/impl/authority_manager_impl.hpp"

#include <gtest/gtest.h>
#include <boost/endian/conversion.hpp>

#include "mock/core/application/app_state_manager_mock.hpp"
#include "mock/core/blockchain/block_tree_mock.hpp"
#include "mock/core/runtime/grandpa_api_mock.hpp"
#include "storage/in_memory/in_memory_spaced_storage.hpp"

#include "testutil/prepare_loggers.hpp"

using kagome::application::AppStateManagerMock;
using kagome::blockchain::BlockTreeMock;
using kagome::consensus::grandpa::AuthorityManagerImpl;
using kagome::primitives::AuthoritySet;
using kagome::primitives::AuthoritySetId;
using kagome::primitives::BlockHash;
using kagome::primitives::BlockHeader;
using kagome::primitives::BlockNumber;
using kagome::primitives::Consensus;
using kagome::primitives::events::ChainSubscriptionEngine;
using kagome::runtime::GrandpaApiMock;
using kagome::storage::InMemorySpacedStorage;
using testing::_;
using testing::Return;
using testing::ReturnRefOfCopy;

BlockHash mockHash(uint64_t number) {
  BlockHash hash;
  boost::endian::store_little_u64(hash.data(), number);
  return hash;
}
BlockNumber mockHashNumber(const BlockHash &hash) {
  return boost::endian::load_little_u64(hash.data());
}

class AuthorityManagerTest : public testing::Test {
 public:
  static void SetUpTestCase() {
    testutil::prepareLoggers();
  }

  void SetUp() override {
    EXPECT_CALL(*app_state_manager, atPrepare(_));

    EXPECT_CALL(*block_tree, getGenesisBlockHash())
        .WillRepeatedly(ReturnRefOfCopy(mockHash(0)));
    EXPECT_CALL(*block_tree, hasBlockHeader(_)).WillRepeatedly(Return(true));
    EXPECT_CALL(*block_tree, getDescendingChainToBlock(_, _))
        .WillRepeatedly([](BlockHash hash, uint32_t count) {
          std::vector<BlockHash> hashes;
          auto number = mockHashNumber(hash);
          while (true) {
            hashes.emplace_back(mockHash(number));
            if (number == 0) {
              break;
            }
            --number;
          }
          return hashes;
        });
    EXPECT_CALL(*block_tree, getBlockHeader(_))
        .WillRepeatedly([&](BlockHash hash) {
          BlockHeader header;
          header.number = mockHashNumber(hash);
          if (header.number != 0) {
            header.parent_hash = mockHash(header.number - 1);
          }
          auto it = digests.find(header.number);
          if (it != digests.end()) {
            auto list = voters(it->second.first, it->second.second.has_value())
                            .authorities;
            if (it->second.second) {
              header.digest.emplace_back(
                  Consensus{kagome::primitives::ForcedChange{
                      list,
                      0,
                      *it->second.second,
                  }});
            } else {
              header.digest.emplace_back(
                  Consensus{kagome::primitives::ScheduledChange{list, 0}});
            }
          }
          header.hash_opt = mockHash(header.number);
          return header;
        });
    EXPECT_CALL(*block_tree, isFinalized(_)).Times(testing::AnyNumber());
    // EXPECT_CALL(*block_tree, getBlockJustification(_))
    //     .Times(testing::AnyNumber());
    EXPECT_CALL(*block_tree, getBlockJustification(_))
        .WillRepeatedly(Return(std::error_code{}));

    EXPECT_CALL(*grandpa_api, authorities(mockHash(0)))
        .WillRepeatedly(Return(voters(0).authorities));

    digests[10] = {1, std::nullopt};
    digests[20] = {2, std::nullopt};
    for (auto i = 0; i < 10; ++i) {
      digests[30 + i] = {3 + i, std::nullopt};
    }
    digests[40] = {3, 20};
    digests[50] = {4, 30};

    authority_manager = std::make_shared<AuthorityManagerImpl>(
        app_state_manager,
        block_tree,
        grandpa_api,
        std::make_shared<kagome::storage::InMemorySpacedStorage>(),
        chain_events_engine);
    authority_manager->prepare();
  }

  AuthoritySet voters(AuthoritySetId id, bool forced = false) {
    auto authority = forced ? 1000000 + id : id;
    return {id, {{{mockHash(authority)}, authority}}};
  }

  AuthoritySet query(BlockNumber at, bool next) {
    return *authority_manager
                ->authorities(
                    {at, mockHash(at)},
                    kagome::consensus::grandpa::IsBlockFinalized{next})
                .value();
  }

  std::shared_ptr<AppStateManagerMock> app_state_manager =
      std::make_shared<AppStateManagerMock>();
  std::shared_ptr<BlockTreeMock> block_tree = std::make_shared<BlockTreeMock>();
  std::shared_ptr<GrandpaApiMock> grandpa_api =
      std::make_shared<GrandpaApiMock>();
  std::shared_ptr<ChainSubscriptionEngine> chain_events_engine =
      std::make_shared<ChainSubscriptionEngine>();
  std::shared_ptr<AuthorityManagerImpl> authority_manager;
  std::map<BlockNumber, std::pair<AuthoritySetId, std::optional<BlockNumber>>>
      digests;
};

TEST_F(AuthorityManagerTest, Genesis) {
  EXPECT_EQ(query(0, true), voters(0));
}

TEST_F(AuthorityManagerTest, Scheduled) {
  EXPECT_EQ(query(10, false), voters(0));
  EXPECT_EQ(query(10, true), voters(1));
  EXPECT_EQ(query(20, false), voters(1));
  EXPECT_EQ(query(20, true), voters(2));
}

TEST_F(AuthorityManagerTest, Forced) {
  EXPECT_EQ(query(39, true), voters(12));
  EXPECT_EQ(query(40, true), voters(3, true));
  EXPECT_EQ(query(50, true), voters(4, true));
}
