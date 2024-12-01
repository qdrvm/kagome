/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "core/parachain/parachain_test_harness.hpp"
#include "parachain/validator/parachain_processor.hpp"

using namespace kagome::parachain;
namespace runtime = kagome::runtime;

class BackingTest : public ProspectiveParachainsTestHarness {
  void SetUp() override {
    ProspectiveParachainsTestHarness::SetUp();
    parachain_api_ = std::make_shared<runtime::ParachainHostMock>();
  }

  void TearDown() override {
    parachain_api_.reset();
    ProspectiveParachainsTestHarness::TearDown();
  }

 public:
  std::shared_ptr<runtime::ParachainHostMock> parachain_api_;
  struct TestState {
    std::vector<ParachainId> chain_ids;

    TestState() {
      const ParachainId chain_a(1);
      const ParachainId chain_b(2);

      chain_ids = {chain_a, chain_b};
    }
  };

  struct TestLeaf {
    BlockNumber number;
    Hash hash;
    std::vector<std::pair<ParachainId, uint32_t>> min_relay_parents;
  };

  void activate_leaf(const TestLeaf &leaf, const TestState &test_state) {
    const auto &[leaf_number, leaf_hash, min_relay_parents] = leaf;
    network::ExView update{
        .view = {},
        .new_head =
            BlockHeader{
                .number = leaf_number,
                .parent_hash = get_parent_hash(leaf_hash),
                .state_root = {},
                .extrinsics_root = {},
                .digest = {},
                .hash_opt = {},
            },
        .lost = {},
    };
    update.new_head.hash_opt = leaf_hash;

    EXPECT_CALL(*parachain_api_, staging_async_backing_params(leaf_hash))
        .WillRepeatedly(Return(outcome::success(fragment::AsyncBackingParams{
            .max_candidate_depth = 4, .allowed_ancestry_len = 3})));

    const BlockNumber min_min = [&]() -> BlockNumber {
      std::optional<BlockNumber> min_min;
      for (const auto &[_, block_num] : min_relay_parents) {
        min_min = (min_min ? std::min(*min_min, block_num) : block_num);
      }
      if (min_min) {
        return *min_min;
      }
      return leaf_number;
    }();

    const auto ancestry_len = leaf_number + 1 - min_min;
    std::vector<Hash> ancestry_hashes;
    std::vector<BlockNumber> ancestry_numbers;

    Hash d = leaf_hash;
    for (BlockNumber x = 0; x < ancestry_len; ++x) {
      ancestry_hashes.emplace_back(d);
      ancestry_numbers.push_back(leaf_number - x);
      d = get_parent_hash(d);
    }
    ASSERT_EQ(ancestry_hashes.size(), ancestry_numbers.size());

    // std::cout << "---------------------------" << std::endl;
    // for(size_t i = 0; i < ancestry_hashes.size(); ++i) {
    //     const auto &h = ancestry_hashes[i];
    //     const auto &n = ancestry_numbers[i];
    //
    //    std::cout <<
    //        fmt::format("{}  {}", n, h) << std::endl;
    //}

	size_t requested_len = 0;
    for (size_t i = 0; i < ancestry_hashes.size(); ++i) {
      const auto &hash = ancestry_hashes[i];
      const auto &number = ancestry_numbers[i];
      const auto parent_hash =
          ((i == ancestry_hashes.size() - 1) ? get_parent_hash(hash)
                                             : ancestry_hashes[i + 1]);

      EXPECT_CALL(*block_tree_, getBlockHeader(hash))
          .WillRepeatedly(Return(BlockHeader{
              .number = number,
              .parent_hash = parent_hash,
              .state_root = {},
              .extrinsics_root = {},
              .digest = {},
              .hash_opt = {},
          }));

			if (requested_len == 0) {
				assert_matches!(
					virtual_overseer.recv().await,
					AllMessages::ProspectiveParachains(
						ProspectiveParachainsMessage::GetMinimumRelayParents(parent, tx)
					) if parent == leaf_hash => {
						tx.send(min_relay_parents.clone()).unwrap();
					}
				);
			}

			requested_len += 1;
    }
  }
};

TEST_F(BackingTest, seconding_sanity_check_allowed_on_all) {
  TestState test_state;

  const BlockNumber LEAF_A_BLOCK_NUMBER = 100;
  const BlockNumber LEAF_A_ANCESTRY_LEN = 3;
  const auto para_id = test_state.chain_ids[0];

  // `a` is grandparent of `b`.
  const auto leaf_a_hash = fromNumber(130);
  const auto leaf_a_parent = get_parent_hash(leaf_a_hash);
  // const auto activated = new_leaf(leaf_a_hash, LEAF_A_BLOCK_NUMBER);
  const TestLeaf test_leaf_a{
      .number = LEAF_A_BLOCK_NUMBER,
      .hash = leaf_a_hash,
      .min_relay_parents = {{para_id,
                             LEAF_A_BLOCK_NUMBER - LEAF_A_ANCESTRY_LEN}},
  };

  const BlockNumber LEAF_B_BLOCK_NUMBER = LEAF_A_BLOCK_NUMBER + 2;
  const BlockNumber LEAF_B_ANCESTRY_LEN = 4;

  const auto leaf_b_hash = fromNumber(128);
  // let activated = new_leaf(, );
  const TestLeaf test_leaf_b{
      .number = LEAF_B_BLOCK_NUMBER,
      .hash = leaf_b_hash,
      .min_relay_parents = {{para_id,
                             LEAF_B_BLOCK_NUMBER - LEAF_B_ANCESTRY_LEN}},
  };

  activate_leaf(test_leaf_a, test_state);
  activate_leaf(test_leaf_b, test_state);
}
