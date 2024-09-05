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

  HashSet<CandidateHash> get_unconnected(const FragmentChain &chain) {
    HashSet<CandidateHash> unconnected;
    chain.get_unconnected(
        [&](const auto &c) { unconnected.insert(c.candidate_hash); });
    return unconnected;
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
      const HashSet<CandidateHash> ref = {
          candidate_a_hash, candidate_b_hash, candidate_c_hash};
      ASSERT_EQ(get_unconnected(chain), ref);
    }
  }

  // Various depths
  {
    EXPECT_OUTCOME_TRUE(
        scope,
        Scope::with_ancestors(
            relay_parent_z_info, base_constraints, {}, 0, ancestors));
    {
      const auto chain =
          FragmentChain::init(hasher_, scope, CandidateStorage{});
      ASSERT_TRUE(
          chain.can_add_candidate_as_potential(candidate_a_entry).has_value());
      ASSERT_TRUE(
          chain.can_add_candidate_as_potential(candidate_b_entry).has_value());
      ASSERT_TRUE(
          chain.can_add_candidate_as_potential(candidate_c_entry).has_value());
    }

    {
      const auto chain = populate_chain_from_previous_storage(scope, storage);
      {
        const Vec<CandidateHash> ref = {candidate_a_hash};
        ASSERT_EQ(chain.best_chain_vec(), ref);
      }
      {
        const HashSet<CandidateHash> ref = {candidate_b_hash, candidate_c_hash};
        ASSERT_EQ(get_unconnected(chain), ref);
      }
    }
  }
  {
    // depth is 1, allows two candidates
    EXPECT_OUTCOME_TRUE(
        scope,
        Scope::with_ancestors(
            relay_parent_z_info, base_constraints, {}, 1, ancestors));
    {
      const auto chain =
          FragmentChain::init(hasher_, scope, CandidateStorage{});
      ASSERT_TRUE(
          chain.can_add_candidate_as_potential(candidate_a_entry).has_value());
      ASSERT_TRUE(
          chain.can_add_candidate_as_potential(candidate_b_entry).has_value());
      ASSERT_TRUE(
          chain.can_add_candidate_as_potential(candidate_c_entry).has_value());
    }
    {
      const auto chain = populate_chain_from_previous_storage(scope, storage);
      {
        const Vec<CandidateHash> ref = {candidate_a_hash, candidate_b_hash};
        ASSERT_EQ(chain.best_chain_vec(), ref);
      }
      {
        const HashSet<CandidateHash> ref = {candidate_c_hash};
        ASSERT_EQ(get_unconnected(chain), ref);
      }
    }
  }

  // depth is larger than 2, allows all three candidates
  for (size_t depth = 2; depth < 6; ++depth) {
    EXPECT_OUTCOME_TRUE(
        scope,
        Scope::with_ancestors(
            relay_parent_z_info, base_constraints, {}, depth, ancestors));
    {
      const auto chain =
          FragmentChain::init(hasher_, scope, CandidateStorage{});
      ASSERT_TRUE(
          chain.can_add_candidate_as_potential(candidate_a_entry).has_value());
      ASSERT_TRUE(
          chain.can_add_candidate_as_potential(candidate_b_entry).has_value());
      ASSERT_TRUE(
          chain.can_add_candidate_as_potential(candidate_c_entry).has_value());
    }
    {
      const auto chain = populate_chain_from_previous_storage(scope, storage);
      {
        const Vec<CandidateHash> ref = {
            candidate_a_hash, candidate_b_hash, candidate_c_hash};
        ASSERT_EQ(chain.best_chain_vec(), ref);
      }
      { ASSERT_EQ(chain.unconnected_len(), 0); }
    }
  }

  // Relay parents out of scope
  {
    {
      // Candidate A has relay parent out of scope. Candidates B and C will also
      // be deleted since they form a chain with A.
      Vec<fragment::RelayChainBlockInfo> ancestors_without_x = {
          relay_parent_y_info};
      EXPECT_OUTCOME_TRUE(scope,
                          Scope::with_ancestors(relay_parent_z_info,
                                                base_constraints,
                                                {},
                                                4,
                                                ancestors_without_x));

      const auto chain = populate_chain_from_previous_storage(scope, storage);
      ASSERT_TRUE(chain.best_chain_vec().empty());
      ASSERT_EQ(chain.unconnected_len(), 0);

      ASSERT_EQ(chain.can_add_candidate_as_potential(candidate_a_entry).error(),
                FragmentChain::Error::RELAY_PARENT_NOT_IN_SCOPE);

      // However, if taken independently, both B and C still have potential,
      // since we don't know that A doesn't.
      ASSERT_TRUE(
          chain.can_add_candidate_as_potential(candidate_b_entry).has_value());
      ASSERT_TRUE(
          chain.can_add_candidate_as_potential(candidate_c_entry).has_value());
    }

    {
      // Candidates A and B have relay parents out of scope. Candidate C will
      // also be deleted since it forms a chain with A and B.
      EXPECT_OUTCOME_TRUE(
          scope,
          Scope::with_ancestors(
              relay_parent_z_info, base_constraints, {}, 4, {}));

      const auto chain = populate_chain_from_previous_storage(scope, storage);
      ASSERT_TRUE(chain.best_chain_vec().empty());
      ASSERT_EQ(chain.unconnected_len(), 0);

      ASSERT_EQ(chain.can_add_candidate_as_potential(candidate_a_entry).error(),
                FragmentChain::Error::RELAY_PARENT_NOT_IN_SCOPE);
      ASSERT_EQ(chain.can_add_candidate_as_potential(candidate_b_entry).error(),
                FragmentChain::Error::RELAY_PARENT_NOT_IN_SCOPE);
      // However, if taken independently, C still has potential, since we
      // don't know that A and B don't
      ASSERT_TRUE(
          chain.can_add_candidate_as_potential(candidate_c_entry).has_value());
    }
  }

  // Parachain cycle is not allowed. Make C have the same parent as A.
  {
    fragment::CandidateStorage modified_storage = storage;
    modified_storage.remove_candidate(candidate_c_hash, hasher_);

    const auto &[wrong_pvd_c, wrong_candidate_c] =
        make_committed_candidate(para_id,
                                 relay_parent_z_info.hash,
                                 relay_parent_z_info.number,
                                 {0x0c},
                                 {0x0a},
                                 relay_parent_z_info.number);
    EXPECT_OUTCOME_TRUE(wrong_candidate_c_entry,
                        CandidateEntry::create(
                            network::candidateHash(*hasher_, wrong_candidate_c),
                            wrong_candidate_c,
                            wrong_pvd_c,
                            CandidateState::Backed,
                            hasher_));
    ASSERT_TRUE(modified_storage.add_candidate_entry(wrong_candidate_c_entry)
                    .has_value());

    EXPECT_OUTCOME_TRUE(
        scope,
        Scope::with_ancestors(
            relay_parent_z_info, base_constraints, {}, 4, ancestors));

    {
      const auto chain =
          populate_chain_from_previous_storage(scope, modified_storage);
      {
        const Vec<CandidateHash> ref = {candidate_a_hash, candidate_b_hash};
        ASSERT_EQ(chain.best_chain_vec(), ref);
      }
      ASSERT_EQ(chain.unconnected_len(), 0);

      ASSERT_EQ(
          chain.can_add_candidate_as_potential(wrong_candidate_c_entry).error(),
          FragmentChain::Error::CYCLE);
    }

    {
      // However, if taken independently, C still has potential, since we don't
      // know A and B.
      const auto chain =
          FragmentChain::init(hasher_, scope, CandidateStorage{});
      ASSERT_TRUE(chain.can_add_candidate_as_potential(wrong_candidate_c_entry)
                      .has_value());
    }
  }

  // Candidate C has the same relay parent as candidate A's parent. Relay parent
  // not allowed to move backwards
  {
    fragment::CandidateStorage modified_storage = storage;
    modified_storage.remove_candidate(candidate_c_hash, hasher_);

    const auto &[wrong_pvd_c, wrong_candidate_c] =
        make_committed_candidate(para_id,
                                 relay_parent_x_info.hash,
                                 relay_parent_x_info.number,
                                 {0x0c},
                                 {0x0d},
                                 0);
    EXPECT_OUTCOME_TRUE(wrong_candidate_c_entry,
                        CandidateEntry::create(
                            network::candidateHash(*hasher_, wrong_candidate_c),
                            wrong_candidate_c,
                            wrong_pvd_c,
                            CandidateState::Backed,
                            hasher_));

    ASSERT_TRUE(modified_storage.add_candidate_entry(wrong_candidate_c_entry)
                    .has_value());

    EXPECT_OUTCOME_TRUE(
        scope,
        Scope::with_ancestors(
            relay_parent_z_info, base_constraints, {}, 4, ancestors));

    const auto chain =
        populate_chain_from_previous_storage(scope, modified_storage);
    {
      const Vec<CandidateHash> ref = {candidate_a_hash, candidate_b_hash};
      ASSERT_EQ(chain.best_chain_vec(), ref);
    }
    ASSERT_EQ(chain.unconnected_len(), 0);

    ASSERT_EQ(
        chain.can_add_candidate_as_potential(wrong_candidate_c_entry).error(),
        FragmentChain::Error::RELAY_PARENT_MOVED_BACKWARDS);
  }

  // Candidate C is an unconnected candidate.
  // C's relay parent is allowed to move backwards from B's relay parent,
  // because C may later on trigger a reorg and B may get removed.
  {
    fragment::CandidateStorage modified_storage = storage;
    modified_storage.remove_candidate(candidate_c_hash, hasher_);

    const auto &[unconnected_pvd_c, unconnected_candidate_c] =
        make_committed_candidate(para_id,
                                 relay_parent_x_info.hash,
                                 relay_parent_x_info.number,
                                 {0x0d},
                                 {0x0e},
                                 0);
    const auto unconnected_candidate_c_hash =
        network::candidateHash(*hasher_, unconnected_candidate_c);
    EXPECT_OUTCOME_TRUE(unconnected_candidate_c_entry,
                        CandidateEntry::create(unconnected_candidate_c_hash,
                                               unconnected_candidate_c,
                                               unconnected_pvd_c,
                                               CandidateState::Backed,
                                               hasher_));

    ASSERT_TRUE(
        modified_storage.add_candidate_entry(unconnected_candidate_c_entry)
            .has_value());

    {
      EXPECT_OUTCOME_TRUE(
          scope,
          Scope::with_ancestors(
              relay_parent_z_info, base_constraints, {}, 4, ancestors));
      {
        const auto chain =
            FragmentChain::init(hasher_, scope, CandidateStorage{});
        ASSERT_TRUE(
            chain.can_add_candidate_as_potential(unconnected_candidate_c_entry)
                .has_value());
      }

      {
        const auto chain =
            populate_chain_from_previous_storage(scope, modified_storage);
        {
          const Vec<CandidateHash> ref = {candidate_a_hash, candidate_b_hash};
          ASSERT_EQ(chain.best_chain_vec(), ref);
        }
        {
          const HashSet<CandidateHash> ref = {unconnected_candidate_c_hash};
          ASSERT_EQ(get_unconnected(chain), ref);
        }
      }
    }

    // Candidate A is a pending availability candidate and Candidate C is an
    // unconnected candidate, C's relay parent is not allowed to move backwards
    // from A's relay parent because we're sure A will not get removed in the
    // future, as it's already on-chain (unless it times out availability, a
    // case for which we don't care to optimise for)
    modified_storage.remove_candidate(candidate_a_hash, hasher_);
    const auto &[modified_pvd_a, modified_candidate_a] =
        make_committed_candidate(para_id,
                                 relay_parent_y_info.hash,
                                 relay_parent_y_info.number,
                                 {0x0a},
                                 {0x0b},
                                 relay_parent_y_info.number);

    const auto modified_candidate_a_hash =
        network::candidateHash(*hasher_, modified_candidate_a);
    EXPECT_OUTCOME_TRUE(modified_candidate_a_entry,
                        CandidateEntry::create(modified_candidate_a_hash,
                                               modified_candidate_a,
                                               modified_pvd_a,
                                               CandidateState::Backed,
                                               hasher_));
    ASSERT_TRUE(modified_storage.add_candidate_entry(modified_candidate_a_entry)
                    .has_value());
    {
      EXPECT_OUTCOME_TRUE(
          scope,
          Scope::with_ancestors(relay_parent_z_info,
                                base_constraints,
                                {PendingAvailability{
                                    .candidate_hash = modified_candidate_a_hash,
                                    .relay_parent = relay_parent_y_info,
                                }},
                                4,
                                ancestors));

      const auto chain =
          populate_chain_from_previous_storage(scope, modified_storage);
      {
        const Vec<CandidateHash> ref = {modified_candidate_a_hash,
                                        candidate_b_hash};
        ASSERT_EQ(chain.best_chain_vec(), ref);
      }
      ASSERT_EQ(chain.unconnected_len(), 0);
      ASSERT_EQ(
          chain.can_add_candidate_as_potential(unconnected_candidate_c_entry)
              .error(),
          FragmentChain::Error::
              RELAY_PARENT_PRECEDES_CANDIDATE_PENDING_AVAILABILITY);
    }

    // Not allowed to fork from a candidate pending availability
    const auto &[wrong_pvd_c, wrong_candidate_c] =
        make_committed_candidate(para_id,
                                 relay_parent_y_info.hash,
                                 relay_parent_y_info.number,
                                 {0x0a},
                                 {0x0b2},
                                 0);

    const auto wrong_candidate_c_hash =
        network::candidateHash(*hasher_, wrong_candidate_c);
    EXPECT_OUTCOME_TRUE(wrong_candidate_c_entry,
                        CandidateEntry::create(wrong_candidate_c_hash,
                                               wrong_candidate_c,
                                               wrong_pvd_c,
                                               CandidateState::Backed,
                                               hasher_));
    ASSERT_TRUE(modified_storage.add_candidate_entry(wrong_candidate_c_entry)
                    .has_value());

    // Does not even matter if the fork selection rule would have picked up the
    // new candidate, as the other is already pending availability.
    ASSERT_TRUE(FragmentChain::fork_selection_rule(wrong_candidate_c_hash,
                                                   modified_candidate_a_hash));
    {
      EXPECT_OUTCOME_TRUE(
          scope,
          Scope::with_ancestors(relay_parent_z_info,
                                base_constraints,
                                {PendingAvailability{
                                    .candidate_hash = modified_candidate_a_hash,
                                    .relay_parent = relay_parent_y_info,
                                }},
                                4,
                                ancestors));

      const auto chain =
          populate_chain_from_previous_storage(scope, modified_storage);
      {
        const Vec<CandidateHash> ref = {modified_candidate_a_hash,
                                        candidate_b_hash};
        ASSERT_EQ(chain.best_chain_vec(), ref);
      }
      ASSERT_EQ(chain.unconnected_len(), 0);
      ASSERT_EQ(
          chain.can_add_candidate_as_potential(wrong_candidate_c_entry).error(),
          FragmentChain::Error::FORK_WITH_CANDIDATE_PENDING_AVAILABILITY);
    }
  }
}
