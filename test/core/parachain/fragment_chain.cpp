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

 public:
  FragmentChain populate_chain_from_previous_storage(
      const Scope &scope, const CandidateStorage &storage) {
    FragmentChain chain =
        FragmentChain::init(hasher_, scope, CandidateStorage{});
    FragmentChain prev_chain = chain;
    prev_chain.unconnected = storage;

    chain.populate_from_previous(prev_chain);
    return chain;
  }
};

TEST_F(FragmentChainTest, init_and_populate_from_empty) {
  const auto base_constraints = make_constraints(0, {0}, {0x0a});

  EXPECT_OUTCOME_TRUE(scope,
                      Scope::with_ancestors(
                          RelayChainBlockInfo{
                              .hash = fromNumber(1),
                              .number = 1,
                              .storage_root = fromNumber(2),
                          },
                          base_constraints,
                          {},
                          4,
                          {}));

  auto chain = FragmentChain::init(hasher_, scope, CandidateStorage{});
  ASSERT_EQ(chain.best_chain_len(), 0);
  ASSERT_EQ(chain.unconnected_len(), 0);

  auto new_chain = FragmentChain::init(hasher_, scope, CandidateStorage{});
  new_chain.populate_from_previous(chain);
  ASSERT_EQ(chain.best_chain_len(), 0);
  ASSERT_EQ(chain.unconnected_len(), 0);
}

TEST_F(FragmentChainTest, test_populate_and_check_potential) {
  fragment::CandidateStorage storage{};

  const ParachainId para_id{5};
  const auto relay_parent_x = fromNumber(1);
  const auto relay_parent_y = fromNumber(2);
  const auto relay_parent_z = fromNumber(3);

  RelayChainBlockInfo relay_parent_x_info{
      .hash = relay_parent_x,
      .number = 0,
      .storage_root = fromNumber(0),
  };
  RelayChainBlockInfo relay_parent_y_info{
      .hash = relay_parent_y,
      .number = 1,
      .storage_root = fromNumber(0),
  };
  RelayChainBlockInfo relay_parent_z_info{
      .hash = relay_parent_z,
      .number = 2,
      .storage_root = fromNumber(0),
  };

  Vec<RelayChainBlockInfo> ancestors = {relay_parent_y_info,
                                        relay_parent_x_info};

  const auto base_constraints = make_constraints(0, {0}, {0x0a});

  // Candidates A -> B -> C. They are all backed
  const auto &[pvd_a, candidate_a] =
      make_committed_candidate(para_id,
                               relay_parent_x_info.hash,
                               relay_parent_x_info.number,
                               {0x0a},
                               {0x0b},
                               relay_parent_x_info.number);
  const auto candidate_a_hash = network::candidateHash(*hasher_, candidate_a);
  const auto candidate_a_entry =
      CandidateEntry::create(
          candidate_a_hash, candidate_a, pvd_a, CandidateState::Backed, hasher_)
          .value();
  ASSERT_TRUE(storage.add_candidate_entry(candidate_a_entry).has_value());

  const auto &[pvd_b, candidate_b] =
      make_committed_candidate(para_id,
                               relay_parent_y_info.hash,
                               relay_parent_y_info.number,
                               {0x0b},
                               {0x0c},
                               relay_parent_y_info.number);
  const auto candidate_b_hash = network::candidateHash(*hasher_, candidate_b);
  const auto candidate_b_entry =
      CandidateEntry::create(
          candidate_b_hash, candidate_b, pvd_b, CandidateState::Backed, hasher_)
          .value();
  ASSERT_TRUE(storage.add_candidate_entry(candidate_b_entry).has_value());

  const auto &[pvd_c, candidate_c] =
      make_committed_candidate(para_id,
                               relay_parent_z_info.hash,
                               relay_parent_z_info.number,
                               {0x0c},
                               {0x0d},
                               relay_parent_z_info.number);
  const auto candidate_c_hash = network::candidateHash(*hasher_, candidate_c);
  const auto candidate_c_entry =
      CandidateEntry::create(
          candidate_c_hash, candidate_c, pvd_c, CandidateState::Backed, hasher_)
          .value();
  ASSERT_TRUE(storage.add_candidate_entry(candidate_c_entry).has_value());

  // Candidate A doesn't adhere to the base constraints.
  for (const auto &wrong_constraints :
       {make_constraints(
            relay_parent_x_info.number, {relay_parent_x_info.number}, {0x0e}),
        make_constraints(relay_parent_y_info.number, {0}, {0x0a})}) {
    EXPECT_OUTCOME_TRUE(
        scope,
        Scope::with_ancestors(
            relay_parent_z_info, wrong_constraints, {}, 4, ancestors));
    auto chain = populate_chain_from_previous_storage(scope, storage);
    ASSERT_TRUE(chain.best_chain_vec().empty());

    if (wrong_constraints.min_relay_parent_number
        == relay_parent_y_info.number) {
      ASSERT_EQ(chain.unconnected_len(), 0);
      ASSERT_EQ(chain.can_add_candidate_as_potential(candidate_a_entry).error(),
                FragmentChain::Error::RELAY_PARENT_NOT_IN_SCOPE);
      ASSERT_TRUE(
          chain.can_add_candidate_as_potential(candidate_b_entry).has_value());
      ASSERT_TRUE(
          chain.can_add_candidate_as_potential(candidate_c_entry).has_value());
    } else {
      HashSet<CandidateHash> unconnected;
      chain.get_unconnected(
          [&](const auto &c) { unconnected.insert(c.candidate_hash); });
      const HashSet<CandidateHash> ref = {
          candidate_a_hash, candidate_b_hash, candidate_c_hash};
      ASSERT_EQ(unconnected, ref);
    }
  }
}
