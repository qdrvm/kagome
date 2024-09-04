/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "parachain/validator/prospective_parachains/scope.hpp"
#include "core/parachain/parachain_test_harness.hpp"

using namespace kagome::parachain::fragment;

class ScopeTest : public ProspectiveParachainsTest {
  void SetUp() override {
    ProspectiveParachainsTest::SetUp();
  }

  void TearDown() override {
    ProspectiveParachainsTest::TearDown();
  }
};

TEST_F(ScopeTest, scope_only_takes_ancestors_up_to_min) {
  RelayChainBlockInfo relay_parent{
      .hash = fromNumber(0),
      .number = 5,
      .storage_root = fromNumber(69),
  };

  Vec<RelayChainBlockInfo> ancestors = {RelayChainBlockInfo{
                                            .hash = fromNumber(4),
                                            .number = 4,
                                            .storage_root = fromNumber(69),
                                        },
                                        RelayChainBlockInfo{
                                            .hash = fromNumber(3),
                                            .number = 3,
                                            .storage_root = fromNumber(69),
                                        },
                                        RelayChainBlockInfo{
                                            .hash = fromNumber(2),
                                            .number = 2,
                                            .storage_root = fromNumber(69),
                                        }};

  const size_t max_depth = 2;
  const auto base_constraints = make_constraints(3, {2}, {1, 2, 3});

  Vec<PendingAvailability> pending_availability;
  auto scope = Scope::with_ancestors(relay_parent,
                                     base_constraints,
                                     pending_availability,
                                     max_depth,
                                     ancestors);
  ASSERT_TRUE(scope.has_value());
  ASSERT_EQ(scope.value().ancestors.size(), 2);
  ASSERT_EQ(scope.value().ancestors_by_hash.size(), 2);
}

TEST_F(ScopeTest, scope_rejects_unordered_ancestors) {
  RelayChainBlockInfo relay_parent{
      .hash = fromNumber(0),
      .number = 5,
      .storage_root = fromNumber(69),
  };

  Vec<RelayChainBlockInfo> ancestors = {RelayChainBlockInfo{
                                            .hash = fromNumber(4),
                                            .number = 4,
                                            .storage_root = fromNumber(69),
                                        },
                                        RelayChainBlockInfo{
                                            .hash = fromNumber(2),
                                            .number = 2,
                                            .storage_root = fromNumber(69),
                                        },
                                        RelayChainBlockInfo{
                                            .hash = fromNumber(3),
                                            .number = 3,
                                            .storage_root = fromNumber(69),
                                        }};

  const size_t max_depth = 2;
  const auto base_constraints = make_constraints(0, {2}, {1, 2, 3});

  Vec<PendingAvailability> pending_availability;
  ASSERT_EQ(Scope::with_ancestors(relay_parent,
                                  base_constraints,
                                  pending_availability,
                                  max_depth,
                                  ancestors)
                .error(),
            Scope::Error::UNEXPECTED_ANCESTOR);
}

TEST_F(ScopeTest, scope_rejects_ancestor_for_0_block) {
  RelayChainBlockInfo relay_parent{
      .hash = fromNumber(0),
      .number = 0,
      .storage_root = fromNumber(69),
  };

  Vec<RelayChainBlockInfo> ancestors = {RelayChainBlockInfo{
      .hash = fromNumber(99),
      .number = 99999,
      .storage_root = fromNumber(69),
  }};

  const size_t max_depth = 2;
  const auto base_constraints = make_constraints(0, {}, {1, 2, 3});

  Vec<PendingAvailability> pending_availability;
  ASSERT_EQ(Scope::with_ancestors(relay_parent,
                                  base_constraints,
                                  pending_availability,
                                  max_depth,
                                  ancestors)
                .error(),
            Scope::Error::UNEXPECTED_ANCESTOR);
}

TEST_F(ScopeTest, scope_rejects_ancestors_that_skip_blocks) {
  RelayChainBlockInfo relay_parent{
      .hash = fromNumber(10),
      .number = 10,
      .storage_root = fromNumber(69),
  };

  Vec<RelayChainBlockInfo> ancestors = {RelayChainBlockInfo{
      .hash = fromNumber(8),
      .number = 8,
      .storage_root = fromNumber(69),
  }};

  const size_t max_depth = 2;
  const auto base_constraints = make_constraints(8, {8, 9}, {1, 2, 3});

  Vec<PendingAvailability> pending_availability;
  ASSERT_EQ(Scope::with_ancestors(relay_parent,
                                  base_constraints,
                                  pending_availability,
                                  max_depth,
                                  ancestors)
                .error(),
            Scope::Error::UNEXPECTED_ANCESTOR);
}
