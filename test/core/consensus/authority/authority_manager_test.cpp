/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "consensus/authority/impl/authority_manager_impl.hpp"
#include "mock/core/application/app_state_manager_mock.hpp"
#include "mock/core/blockchain/block_tree_mock.hpp"
#include "mock/core/storage/persistent_map_mock.hpp"
#include "primitives/digest.hpp"
#include "scale/scale.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"
#include "testutil/outcome/dummy_error.hpp"

using namespace kagome;
using AuthorityManager = authority::AuthorityManagerImpl;
using testing::_;
using testing::Return;

class AuthorityManagerTest : public testing::Test {
 public:
  void SetUp() override {
    app_state_manager = std::make_shared<application::AppStateManagerMock>();

    configuration = std::make_shared<primitives::BabeConfiguration>();
    configuration->genesis_authorities = {
        makeAuthority("GenesisAuthority1", 5),
        makeAuthority("GenesisAuthority2", 10),
        makeAuthority("GenesisAuthority3", 15)};

    block_tree = std::make_shared<blockchain::BlockTreeMock>();

    storage = std::make_shared<StorageMock>();

    EXPECT_CALL(*app_state_manager, atPrepare(_));
    EXPECT_CALL(*app_state_manager, atLaunch(_));
    EXPECT_CALL(*app_state_manager, atShutdown(_));

    auth_mngr_ = std::make_shared<AuthorityManager>(
        app_state_manager, configuration, block_tree, storage);

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

						//
						//                                 - FA - FB - FC
						//                               /   35   40   45
						// GEN - A - B - C - D - E +--- F
						//   1   5   10  15  20  25 \   30
						//                           \
						//                            - EA - EB - EC - ED
						//                              30   35   40   45

            /* A\\D  GEN A  B  C  D  E EA EB EC ED  F FA FB FC */
            /* GEN*/ {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
            /* A	*/ {0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
            /* B	*/ {0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
            /* C	*/ {0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
            /* D	*/ {0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1},
            /* E	*/ {0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1},
            /* EA	*/ {0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0},
            /* EB	*/ {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0},
            /* EC	*/ {0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0},
            /* ED	*/ {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
            /* F	*/ {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1},
            /* FA	*/ {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1},
            /* FB	*/ {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
            /* FC	*/ {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},

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
  }

  using StorageMock =
      storage::face::GenericStorageMock<common::Buffer, common::Buffer>;

  std::shared_ptr<application::AppStateManagerMock> app_state_manager;
  std::shared_ptr<primitives::BabeConfiguration> configuration;
  std::shared_ptr<blockchain::BlockTreeMock> block_tree;
  std::shared_ptr<StorageMock> storage;

  std::shared_ptr<AuthorityManager> auth_mngr_;

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
    EXPECT_CALL(*storage, get(authority::AuthorityManagerImpl::SCHEDULER_TREE))
        .WillOnce(Return(outcome::failure(testutil::DummyError::ERROR)));

    auth_mngr_->prepare();
  }

  /**
   * @brief Check if authorities gotten from the examined block are equal to
   * expected ones
   * @param examining_block
   * @param expected_authorities
   */
  void examine(const primitives::BlockInfo &examining_block,
               const primitives::AuthorityList &expected_authorities) {
    ASSERT_OUTCOME_SUCCESS(actual_authorities_sptr,
                           auth_mngr_->authorities(examining_block));
    const auto &actual_authorities = *actual_authorities_sptr;
    EXPECT_EQ(actual_authorities, expected_authorities);
  }
};

/**
 * @given no initialized manager
 * @when init by data from genesis config
 * @then authorities for any block is equal of authorities from genesis config
 */
TEST_F(AuthorityManagerTest, InitFromGenesis) {
  prepareAuthorityManager();

  examine({20, "D"_hash256}, configuration->genesis_authorities);
}

/**
 * @given no initialized manager, custom authorities saved to storage
 * @when do prepare manager
 * @then authorities for any block is equal of authorities from storage
 */

TEST_F(AuthorityManagerTest, InitFromStorage) {
  // Make custom state
  primitives::AuthorityList custom_authorities{
      makeAuthority("NonGenesisAuthority", 1)};
  auto node = authority::ScheduleNode::createAsRoot({10, "B"_hash256});
  node->actual_authorities =
      std::make_shared<primitives::AuthorityList>(custom_authorities);
  EXPECT_OUTCOME_SUCCESS(encode_result, scale::encode(node));
  common::Buffer encoded_data(encode_result.value());

  EXPECT_CALL(*storage, get(authority::AuthorityManagerImpl::SCHEDULER_TREE))
      .WillOnce(Return(encoded_data));

  auth_mngr_->prepare();

  examine({20, "D"_hash256}, custom_authorities);
}

/**
 * @given initialized manager has some state
 * @when do finalize for some block
 * @then aclual state will be saved to storage
 */
TEST_F(AuthorityManagerTest, OnFinalize) {
  prepareAuthorityManager();

  EXPECT_OUTCOME_SUCCESS(authorities_result,
                         auth_mngr_->authorities({10, "B"_hash256}));

  auto &orig_authorities = *authorities_result.value();

  // Make expected state
  auto node = authority::ScheduleNode::createAsRoot({20, "D"_hash256});
  node->actual_authorities =
      std::make_shared<primitives::AuthorityList>(orig_authorities);

  EXPECT_OUTCOME_SUCCESS(encode_result, scale::encode(node));
  common::Buffer encoded_data(std::move(encode_result.value()));

  ON_CALL(*storage, put_rv(_, _))
      .WillByDefault(testing::Invoke([&encoded_data](auto &key, auto &val) {
        EXPECT_EQ(key, authority::AuthorityManagerImpl::SCHEDULER_TREE);
        EXPECT_EQ(val, encoded_data);
        return outcome::success();
      }));

  EXPECT_CALL(*storage, put_rv(_, _)).WillOnce(Return(outcome::success()));

  EXPECT_OUTCOME_SUCCESS(finalisation_result,
                         auth_mngr_->onFinalize({20, "D"_hash256}));

  examine({30, "F"_hash256}, orig_authorities);
}

/**
 * @given initialized manager has some state
 * @when apply Consensus message as ScheduledChange
 * @then actual state was not change before finalize and change after finalize
 * if delay passed (only for block with number of target block number +
 * subchain_lenght)
 */
TEST_F(AuthorityManagerTest, OnConsensus_ScheduledChange) {
  prepareAuthorityManager();

  EXPECT_OUTCOME_SUCCESS(old_auth_r,
                         auth_mngr_->authorities({20, "D"_hash256}));
  auto &old_authorities = *old_auth_r.value();

  auto engine_id = primitives::kBabeEngineId;
  primitives::BlockInfo target_block{5, "A"_hash256};
  primitives::AuthorityList new_authorities{makeAuthority("Auth1", 123)};
  uint32_t subchain_lenght = 10;

  EXPECT_OUTCOME_SUCCESS(
      r1,
      auth_mngr_->onConsensus(
          engine_id,
          target_block,
          primitives::ScheduledChange(new_authorities, subchain_lenght)));

  examine({5, "A"_hash256}, old_authorities);
  examine({10, "B"_hash256}, old_authorities);
  examine({15, "C"_hash256}, old_authorities);
  examine({20, "D"_hash256}, old_authorities);
  examine({25, "E"_hash256}, old_authorities);

  EXPECT_CALL(*storage, put_rv(_, _)).WillOnce(Return(outcome::success()));
  EXPECT_OUTCOME_SUCCESS(finalisation_result,
                         auth_mngr_->onFinalize({20, "D"_hash256}));

  examine({20, "D"_hash256}, new_authorities);
  examine({25, "E"_hash256}, new_authorities);
}

/**
 * @given initialized manager has some state
 * @when apply Consensus message as ForcedChange
 * @then actual state was change after delay passed (only for block with number
 * of target block number + subchain_lenght)
 */
TEST_F(AuthorityManagerTest, OnConsensus_ForcedChange) {
  prepareAuthorityManager();

  EXPECT_OUTCOME_SUCCESS(old_auth_r,
                         auth_mngr_->authorities({35, "F"_hash256}));
  auto &old_authorities = *old_auth_r.value();

  auto engine_id = primitives::kBabeEngineId;
  primitives::BlockInfo target_block{10, "B"_hash256};
  primitives::AuthorityList new_authorities{makeAuthority("Auth1", 123)};
  uint32_t subchain_lenght = 10;

  EXPECT_OUTCOME_SUCCESS(
      r1,
      auth_mngr_->onConsensus(
          engine_id,
          target_block,
          primitives::ForcedChange(new_authorities, subchain_lenght)));

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
 */
TEST_F(AuthorityManagerTest, OnConsensus_DisableAuthority) {
  prepareAuthorityManager();

  EXPECT_OUTCOME_SUCCESS(old_authorities_result,
                         auth_mngr_->authorities({35, "F"_hash256}));
  auto &old_authorities = *old_authorities_result.value();

  auto engine_id = primitives::kBabeEngineId;
  primitives::BlockInfo target_block{10, "B"_hash256};
  size_t authority_index = 1;

  primitives::AuthorityList new_authorities = old_authorities;
  assert(new_authorities.size() == 3);
  new_authorities[authority_index].weight = 0;

  EXPECT_OUTCOME_SUCCESS(
      r1,
      auth_mngr_->onConsensus(
          engine_id, target_block, primitives::OnDisabled({authority_index})));

  examine({5, "A"_hash256}, old_authorities);
  examine({10, "B"_hash256}, new_authorities);
  examine({15, "C"_hash256}, new_authorities);
}

/**
 * @given initialized manager has some state
 * @when apply Consensus message as ScheduledChange
 * @then actual state was not change before finalize and changed (disabled)
 * after finalize if delay passed (only for block with number of target block
 * number + subchain_lenght)
 */
TEST_F(AuthorityManagerTest, OnConsensus_OnPause) {
  prepareAuthorityManager();

  EXPECT_OUTCOME_SUCCESS(old_authorities_result,
                         auth_mngr_->authorities({35, "F"_hash256}));
  auto &old_authorities = *old_authorities_result.value();

  auto engine_id = primitives::kBabeEngineId;
  primitives::BlockInfo target_block{5, "A"_hash256};
  uint32_t delay = 10;

  EXPECT_OUTCOME_SUCCESS(
      r1,
      auth_mngr_->onConsensus(
          engine_id, target_block, primitives::Pause(delay)));

  primitives::AuthorityList new_authorities = old_authorities;
  for (auto &authority : new_authorities) {
    authority.weight = 0;
  }

  examine({5, "A"_hash256}, old_authorities);
  examine({10, "B"_hash256}, old_authorities);
  examine({15, "C"_hash256}, old_authorities);
  examine({20, "D"_hash256}, old_authorities);
  examine({25, "E"_hash256}, old_authorities);

  EXPECT_CALL(*storage, put_rv(_, _)).WillOnce(Return(outcome::success()));
  EXPECT_OUTCOME_SUCCESS(finalisation_result,
                         auth_mngr_->onFinalize({20, "D"_hash256}));

  examine({20, "D"_hash256}, new_authorities);
  examine({25, "E"_hash256}, new_authorities);
}

/**
 * @given initialized manager has some state
 * @when apply Consensus message as ForcedChange
 * @then actual state was change (enabled again) after delay passed (only for
 * block with number of target block number + subchain_lenght)
 */

TEST_F(AuthorityManagerTest, OnConsensus_OnResume) {
  prepareAuthorityManager();

  EXPECT_OUTCOME_SUCCESS(old_authorities_result,
                         auth_mngr_->authorities({35, "F"_hash256}));
  auto &enabled_authorities = *old_authorities_result.value();

  primitives::AuthorityList disabled_authorities = enabled_authorities;
  for (auto &authority : disabled_authorities) {
    authority.weight = 0;
  }

  ASSERT_NE(enabled_authorities, disabled_authorities);

  {
    auto engine_id = primitives::kBabeEngineId;
    primitives::BlockInfo target_block{5, "A"_hash256};
    uint32_t delay = 5;

    EXPECT_OUTCOME_SUCCESS(
        r1,
        auth_mngr_->onConsensus(
            engine_id, target_block, primitives::Pause(delay)));

    EXPECT_CALL(*storage, put_rv(_, _)).WillOnce(Return(outcome::success()));
    EXPECT_OUTCOME_SUCCESS(finalisation_result,
                           auth_mngr_->onFinalize({10, "B"_hash256}));
  }

  examine({10, "B"_hash256}, disabled_authorities);
  examine({15, "C"_hash256}, disabled_authorities);
  examine({20, "D"_hash256}, disabled_authorities);
  examine({25, "E"_hash256}, disabled_authorities);

  {
    auto engine_id = primitives::kBabeEngineId;
    primitives::BlockInfo target_block{15, "C"_hash256};
    uint32_t delay = 10;

    EXPECT_OUTCOME_SUCCESS(
        r1,
        auth_mngr_->onConsensus(
            engine_id, target_block, primitives::Resume(delay)));
  }

  examine({10, "B"_hash256}, disabled_authorities);
  examine({15, "C"_hash256}, disabled_authorities);
  examine({20, "D"_hash256}, disabled_authorities);
  examine({25, "E"_hash256}, enabled_authorities);
}
