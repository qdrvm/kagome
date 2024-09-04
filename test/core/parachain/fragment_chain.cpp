/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "parachain/validator/prospective_parachains/fragment_chain.hpp"
#include "core/parachain/parachain_test_harness.hpp"

using namespace kagome::parachain::fragment;

class FragmentChainTest : public ProspectiveParachainsTest {
  void SetUp() override {
    ProspectiveParachainsTest::SetUp();
  }

  void TearDown() override {
    ProspectiveParachainsTest::TearDown();
  }
};

TEST_F(FragmentChainTest, init_and_populate_from_empty) {
  const auto base_constraints = make_constraints(0, {0}, {0x0a});

  auto scope_res = Scope::with_ancestors(
      RelayChainBlockInfo{
          .hash = fromNumber(1),
          .number = 1,
          .storage_root = fromNumber(2),
      },
      base_constraints,
      {},
      4,
      {});
  ASSERT_TRUE(scope_res.has_value());
  auto &scope = scope_res.value();

  auto chain = FragmentChain::init(hasher_, scope, CandidateStorage{});

  ASSERT_EQ(chain.best_chain_len(), 0);
  ASSERT_EQ(chain.unconnected_len(), 0);

  auto new_chain = FragmentChain::init(hasher_, scope, CandidateStorage{});
  new_chain.populate_from_previous(chain);
  ASSERT_EQ(chain.best_chain_len(), 0);
  ASSERT_EQ(chain.unconnected_len(), 0);
}
