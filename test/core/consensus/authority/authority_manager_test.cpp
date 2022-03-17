/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/authority/impl/authority_manager_impl.hpp"

#include <gtest/gtest.h>

#include "consensus/authority/impl/schedule_node.hpp"
#include "mock/core/application/app_state_manager_mock.hpp"
#include "mock/core/blockchain/block_tree_mock.hpp"
#include "mock/core/crypto/hasher_mock.hpp"
#include "mock/core/runtime/grandpa_api_mock.hpp"
#include "mock/core/storage/persistent_map_mock.hpp"
#include "mock/core/storage/trie/trie_batches_mock.hpp"
#include "mock/core/storage/trie/trie_storage_mock.hpp"
#include "primitives/digest.hpp"
#include "scale/scale.hpp"
#include "storage/predefined_keys.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"
#include "testutil/outcome/dummy_error.hpp"
#include "testutil/prepare_loggers.hpp"

using namespace kagome;
using authority::AuthorityManagerImpl;
using kagome::storage::trie::EphemeralTrieBatchMock;
using primitives::AuthorityList;
using testing::_;
using testing::Return;

// TODO (kamilsa): workaround unless we bump gtest version to 1.8.1+
namespace kagome::primitives {
  std::ostream &operator<<(std::ostream &s,
                           const detail::DigestItemCommon &dic) {
    return s;
  }
  std::ostream &operator<<(std::ostream &s, const ChangesTrieSignal &) {
    return s;
  }
}  // namespace kagome::primitives

class AuthorityManagerTest : public testing::Test {
 public:
  static void SetUpTestCase() {
    testutil::prepareLoggers();
  }

  void SetUp() override {
    app_state_manager = std::make_shared<application::AppStateManagerMock>();

    authorities = std::make_shared<AuthorityList>(0);
    authorities->emplace_back(makeAuthority("GenesisAuthority1", 5));
    authorities->emplace_back(makeAuthority("GenesisAuthority2", 10));
    authorities->emplace_back(makeAuthority("GenesisAuthority3", 15));

    block_tree = std::make_shared<blockchain::BlockTreeMock>();

    storage = std::make_shared<storage::trie::TrieStorageMock>();
    EXPECT_CALL(*storage, getEphemeralBatchAt(_))
        .WillRepeatedly(testing::Invoke([] {
          auto batch = std::make_unique<EphemeralTrieBatchMock>();
          EXPECT_CALL(*batch, tryGet(_))
              .WillRepeatedly(
                  Return(storage::Buffer::fromHex("0000000000000000").value()));
          return batch;
        }));

    grandpa_api = std::make_shared<runtime::GrandpaApiMock>();
    EXPECT_CALL(*grandpa_api, authorities(_))
        .WillRepeatedly(Return(*authorities));

    hasher = std::make_shared<crypto::HasherMock>();
    EXPECT_CALL(*hasher, twox_128(_)).WillRepeatedly(Return(common::Hash128{}));

    EXPECT_CALL(*app_state_manager, atPrepare(_));

    authority_manager =
        std::make_shared<AuthorityManagerImpl>(AuthorityManagerImpl::Config{},
                                               app_state_manager,
                                               block_tree,
                                               storage,
                                               grandpa_api,
                                               hasher);

    ON_CALL(*block_tree, hasDirectChain(_, _))
        .WillByDefault(testing::Invoke([](auto &anc, auto &des) {
          static std::map<primitives::BlockHash, uint8_t> mapping = {
              {"GEN"_hash256, 0},
              {"A"_hash256, 1},
              {"B"_hash256, 2},
              {"C"_hash256, 3},
              {"D"_hash256, 4},
              {"E"_hash256, 5},
              {"EA"_hash256, 6},
              {"EB"_hash256, 7},
              {"EC"_hash256, 8},
              {"ED"_hash256, 9},
              {"F"_hash256, 10},
              {"FA"_hash256, 11},
              {"FB"_hash256, 12},
              {"FC"_hash256, 13},
          };
          static bool ancestry[][14] = {
              // clang-format off
	    /*
	                                         - FA - FB - FC
	                                       /   35   40   45
	         GEN - A - B - C - D - E +--- F
	           1   5   10  15  20  25 \   30
	                                   \
	                                    - EA - EB - EC - ED
	                                      30   35   40   45
            */
            /* A\\D  GEN A  B  C  D  E EA EB EC ED  F FA FB FC */
            /* GEN*/ {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
            /* A  */ {0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
            /* B  */ {0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
            /* C  */ {0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
            /* D  */ {0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1},
            /* E  */ {0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1},
            /* EA */ {0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0},
            /* EB */ {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0},
            /* EC */ {0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0},
            /* ED */ {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
            /* F  */ {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1},
            /* FA */ {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1},
            /* FB */ {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
            /* FC */ {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},

              // clang-format on
          };
          if (auto ai = mapping.find(anc); ai != mapping.end()) {
            if (auto di = mapping.find(des); di != mapping.end()) {
              return ancestry[ai->second][di->second];
            } else {
              return false;
            }
          }
          throw std::runtime_error("Broken test");
        }));
    EXPECT_CALL(*block_tree, hasDirectChain(_, _)).Times(testing::AnyNumber());

    EXPECT_CALL(*block_tree, getBlockHeader(primitives::BlockId("GEN"_hash256)))
        .WillRepeatedly(Return(primitives::BlockHeader{}));
  }

  const primitives::BlockInfo genesis_block{0, "GEN"_hash256};
  const std::vector<primitives::BlockHash> leaves{genesis_block.hash};

  std::shared_ptr<application::AppStateManagerMock> app_state_manager;
  std::shared_ptr<blockchain::BlockTreeMock> block_tree;
  std::shared_ptr<storage::trie::TrieStorageMock> storage;
  std::shared_ptr<runtime::GrandpaApiMock> grandpa_api;
  std::shared_ptr<crypto::HasherMock> hasher;
  std::shared_ptr<AuthorityManagerImpl> authority_manager;
  std::shared_ptr<AuthorityList> authorities;

  primitives::Authority makeAuthority(std::string_view id, uint32_t weight) {
    primitives::Authority authority;
    std::copy_n(id.begin(),
                std::min(id.size(), authority.id.id.size()),
                authority.id.id.begin());
    authority.weight = weight;
    return authority;
  }

  /// Init by data from genesis config
  void prepareAuthorityManager() {
    auto node = authority::ScheduleNode::createAsRoot(genesis_block);
    node->actual_authorities = authorities;
    EXPECT_OUTCOME_SUCCESS(encode_result, scale::encode(node));
    common::Buffer encoded_data(encode_result.value());

    EXPECT_CALL(*block_tree, getLastFinalized())
        .WillRepeatedly(Return(genesis_block));

    EXPECT_CALL(*block_tree, getLeaves()).WillOnce(Return(leaves));

    authority_manager->prepare();
  }

  /**
   * @brief Check if authorities gotten from the examined block are equal to
   * expected ones
   * @param examining_block
   * @param expected_authorities
   */
  void examine(const primitives::BlockInfo &examining_block,
               const primitives::AuthorityList &expected_authorities) {
    ASSERT_OUTCOME_SUCCESS(
        actual_authorities_sptr,
        authority_manager->authorities(examining_block, false));
    const auto &actual_authorities = *actual_authorities_sptr;
    EXPECT_EQ(actual_authorities, expected_authorities);
  }
};

/**
 * @given no initialized manager
 * @when init basing actual blockchain state
 * @then authorities for any block is equal of authorities from genesis config
 */
TEST_F(AuthorityManagerTest, Init) {
  prepareAuthorityManager();

  examine({20, "D"_hash256}, *authorities);
}

/**
 * @given initialized manager has some state
 * @when do pruning upto block
 * @then actual state will be saved to storage
 */
TEST_F(AuthorityManagerTest, Prune) {
  prepareAuthorityManager();

  EXPECT_OUTCOME_SUCCESS(
      authorities_result,
      authority_manager->authorities({10, "B"_hash256}, true));

  auto &orig_authorities = *authorities_result.value();

  // Make expected state
  auto node = authority::ScheduleNode::createAsRoot({20, "D"_hash256});
  node->actual_authorities =
      std::make_shared<primitives::AuthorityList>(orig_authorities);

  EXPECT_OUTCOME_SUCCESS(encode_result, scale::encode(node));
  common::Buffer encoded_data(std::move(encode_result.value()));

  authority_manager->prune({20, "D"_hash256});

  examine({30, "F"_hash256}, orig_authorities);
}

/**
 * @given initialized manager has some state
 * @when apply Consensus message as ScheduledChange
 * @then actual state was not change before finalize and change after finalize
 * if delay passed (only for block with number of target block number +
 * subchain_length)
 */
TEST_F(AuthorityManagerTest, OnConsensus_ScheduledChange) {
  prepareAuthorityManager();

  EXPECT_OUTCOME_SUCCESS(
      old_auth_r, authority_manager->authorities({20, "D"_hash256}, true));
  auto &old_authorities = *old_auth_r.value();

  primitives::BlockInfo target_block{5, "A"_hash256};
  primitives::AuthorityList new_authorities{makeAuthority("Auth1", 123)};
  uint32_t subchain_length = 10;

  EXPECT_OUTCOME_SUCCESS(
      r1,
      authority_manager->onConsensus(
          target_block,
          primitives::ScheduledChange(new_authorities, subchain_length)));

  examine({5, "A"_hash256}, old_authorities);
  examine({10, "B"_hash256}, old_authorities);
  examine({15, "C"_hash256}, old_authorities);
  examine({20, "D"_hash256}, old_authorities);
  examine({25, "E"_hash256}, old_authorities);

  authority_manager->prune({20, "D"_hash256});

  examine({20, "D"_hash256}, new_authorities);
  examine({25, "E"_hash256}, new_authorities);
}

/**
 * @given initialized manager has some state
 * @when apply Consensus message as ForcedChange
 * @then actual state was change after delay passed (only for block with number
 * of target block number + subchain_length)
 */
TEST_F(AuthorityManagerTest, OnConsensus_ForcedChange) {
  prepareAuthorityManager();

  EXPECT_OUTCOME_SUCCESS(
      old_auth_r, authority_manager->authorities({35, "F"_hash256}, false));
  auto &old_authorities = *old_auth_r.value();

  primitives::BlockInfo target_block{10, "B"_hash256};
  primitives::AuthorityList new_authorities{makeAuthority("Auth1", 123)};
  uint32_t subchain_length = 10;

  EXPECT_OUTCOME_SUCCESS(
      r1,
      authority_manager->onConsensus(
          target_block,
          primitives::ForcedChange(new_authorities, subchain_length)));

  examine({5, "A"_hash256}, old_authorities);
  examine({10, "B"_hash256}, old_authorities);
  examine({15, "C"_hash256}, old_authorities);
  examine({20, "D"_hash256}, new_authorities);
  examine({25, "E"_hash256}, new_authorities);
}

/**
 * @given initialized manager has some state
 * @when apply Consensus message as DisableAuthority
 * @then actual state was change (disable one of authority) for target block and
 * any one after
 * @note Disabled because this event type wount be used anymore and must be
 * ignored
 */
TEST_F(AuthorityManagerTest, DISABLED_OnConsensus_DisableAuthority) {
  prepareAuthorityManager();

  EXPECT_OUTCOME_SUCCESS(
      old_authorities_result,
      authority_manager->authorities({35, "F"_hash256}, true));
  auto &old_authorities = *old_authorities_result.value();

  primitives::BlockInfo target_block{10, "B"_hash256};
  uint32_t authority_index = 1;

  primitives::AuthorityList new_authorities = old_authorities;
  assert(new_authorities.size() == 3);
  new_authorities[authority_index].weight = 0;

  EXPECT_OUTCOME_SUCCESS(
      r1,
      authority_manager->onConsensus(
          target_block, primitives::OnDisabled({authority_index})));

  examine({5, "A"_hash256}, old_authorities);
  examine({10, "B"_hash256}, new_authorities);
  examine({15, "C"_hash256}, new_authorities);
}

/**
 * @given initialized manager has some state
 * @when apply Consensus message as ScheduledChange
 * @then actual state was not change before finalize and changed (disabled)
 * after finalize if delay passed (only for block with number of target block
 * number + subchain_length)
 */
TEST_F(AuthorityManagerTest, OnConsensus_OnPause) {
  prepareAuthorityManager();

  EXPECT_OUTCOME_SUCCESS(
      old_authorities_result,
      authority_manager->authorities({35, "F"_hash256}, true));
  auto &old_authorities = *old_authorities_result.value();

  primitives::BlockInfo target_block{5, "A"_hash256};
  uint32_t delay = 10;

  EXPECT_OUTCOME_SUCCESS(
      r1,
      authority_manager->onConsensus(target_block, primitives::Pause(delay)));

  primitives::AuthorityList new_authorities = old_authorities;
  for (auto &authority : new_authorities) {
    authority.weight = 0;
  }

  examine({5, "A"_hash256}, old_authorities);
  examine({10, "B"_hash256}, old_authorities);
  examine({15, "C"_hash256}, old_authorities);
  examine({20, "D"_hash256}, old_authorities);
  examine({25, "E"_hash256}, old_authorities);

  authority_manager->prune({20, "D"_hash256});

  examine({20, "D"_hash256}, new_authorities);
  examine({25, "E"_hash256}, new_authorities);
}

/**
 * @given initialized manager has some state
 * @when apply Consensus message as ForcedChange
 * @then actual state was change (enabled again) after delay passed (only for
 * block with number of target block number + subchain_length)
 */

TEST_F(AuthorityManagerTest, OnConsensus_OnResume) {
  prepareAuthorityManager();

  EXPECT_OUTCOME_SUCCESS(
      old_authorities_result,
      authority_manager->authorities({35, "F"_hash256}, true));
  auto &enabled_authorities = *old_authorities_result.value();

  primitives::AuthorityList disabled_authorities = enabled_authorities;
  for (auto &authority : disabled_authorities) {
    authority.weight = 0;
  }

  ASSERT_NE(enabled_authorities, disabled_authorities);

  {
    primitives::BlockInfo target_block{5, "A"_hash256};
    uint32_t delay = 5;

    EXPECT_OUTCOME_SUCCESS(
        r1,
        authority_manager->onConsensus(target_block, primitives::Pause(delay)));

    authority_manager->prune({10, "B"_hash256});
  }

  examine({10, "B"_hash256}, disabled_authorities);
  examine({15, "C"_hash256}, disabled_authorities);
  examine({20, "D"_hash256}, disabled_authorities);
  examine({25, "E"_hash256}, disabled_authorities);

  {
    primitives::BlockInfo target_block{15, "C"_hash256};
    uint32_t delay = 10;

    EXPECT_OUTCOME_SUCCESS(r1,
                           authority_manager->onConsensus(
                               target_block, primitives::Resume(delay)));
  }

  examine({10, "B"_hash256}, disabled_authorities);
  examine({15, "C"_hash256}, disabled_authorities);
  examine({20, "D"_hash256}, disabled_authorities);
  examine({25, "E"_hash256}, enabled_authorities);
}
