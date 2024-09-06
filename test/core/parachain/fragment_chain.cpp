/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "parachain/validator/prospective_parachains/fragment_chain.hpp"
#include <random>
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

  Hash hash(const network::CommittedCandidateReceipt &data) {
    return network::candidateHash(*hasher_, data);
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

  // Test with candidates pending availability
  {
    Vec<PendingAvailability> test_case_0 = {PendingAvailability{
        .candidate_hash = candidate_a_hash,
        .relay_parent = relay_parent_x_info,
    }};
    Vec<PendingAvailability> test_case_1 = {
        PendingAvailability{
            .candidate_hash = candidate_a_hash,
            .relay_parent = relay_parent_x_info,
        },
        PendingAvailability{
            .candidate_hash = candidate_b_hash,
            .relay_parent = relay_parent_y_info,
        }};
    Vec<PendingAvailability> test_case_2 = {
        PendingAvailability{
            .candidate_hash = candidate_a_hash,
            .relay_parent = relay_parent_x_info,
        },
        PendingAvailability{
            .candidate_hash = candidate_b_hash,
            .relay_parent = relay_parent_y_info,
        },
        PendingAvailability{
            .candidate_hash = candidate_c_hash,
            .relay_parent = relay_parent_z_info,
        }};

    for (const auto &pending : {test_case_0, test_case_1, test_case_2}) {
      EXPECT_OUTCOME_TRUE(
          scope,
          Scope::with_ancestors(
              relay_parent_z_info, base_constraints, pending, 3, ancestors));

      const auto chain = populate_chain_from_previous_storage(scope, storage);
      {
        const Vec<CandidateHash> ref = {
            candidate_a_hash, candidate_b_hash, candidate_c_hash};
        ASSERT_EQ(chain.best_chain_vec(), ref);
      }
      ASSERT_EQ(chain.unconnected_len(), 0);
    }

    // Relay parents of pending availability candidates can be out of scope
    // Relay parent of candidate A is out of scope.
    Vec<fragment::RelayChainBlockInfo> ancestors_without_x = {
        relay_parent_y_info};

    {
      EXPECT_OUTCOME_TRUE(
          scope,
          Scope::with_ancestors(relay_parent_z_info,
                                base_constraints,
                                {PendingAvailability{
                                    .candidate_hash = candidate_a_hash,
                                    .relay_parent = relay_parent_x_info,
                                }},
                                4,
                                ancestors_without_x));
      const auto chain = populate_chain_from_previous_storage(scope, storage);
      {
        const Vec<CandidateHash> ref = {
            candidate_a_hash, candidate_b_hash, candidate_c_hash};
        ASSERT_EQ(chain.best_chain_vec(), ref);
      }
      ASSERT_EQ(chain.unconnected_len(), 0);
    }
    {
      // Even relay parents of pending availability candidates which are out of
      // scope cannot move backwards.
      EXPECT_OUTCOME_TRUE(
          scope,
          Scope::with_ancestors(
              relay_parent_z_info,
              base_constraints,
              {PendingAvailability{
                   .candidate_hash = candidate_a_hash,
                   .relay_parent =
                       RelayChainBlockInfo{
                           .hash = relay_parent_x_info.hash,
                           .number = 1,
                           .storage_root = relay_parent_x_info.storage_root,
                       },
               },
               PendingAvailability{
                   .candidate_hash = candidate_b_hash,
                   .relay_parent =
                       RelayChainBlockInfo{
                           .hash = relay_parent_y_info.hash,
                           .number = 0,
                           .storage_root = relay_parent_y_info.storage_root,
                       },
               }},
              4,
              {}));
      const auto chain = populate_chain_from_previous_storage(scope, storage);
      ASSERT_TRUE(chain.best_chain_vec().empty());
      ASSERT_EQ(chain.unconnected_len(), 0);
    }
  }

  // More complex case:
  // max_depth is 2 (a chain of max depth 3).
  // A -> B -> C are the best backable chain.
  // D is backed but would exceed the max depth.
  // F is unconnected and seconded.
  // A1 has same parent as A, is backed but has a higher candidate hash. It'll
  // therefore be deleted.
  //	A1 has underneath a subtree that will all need to be trimmed. A1 -> B1.
  // B1 -> C1
  // 	and B1 -> C2. (C1 is backed).
  // A2 is seconded but is kept because it has a lower candidate hash than A.
  // A2 points to B2, which is backed.
  //
  // Check that D, F, A2 and B2 are kept as unconnected potential candidates.
  {
    {
      EXPECT_OUTCOME_TRUE(
          scope,
          Scope::with_ancestors(
              relay_parent_z_info, base_constraints, {}, 2, ancestors));

      // Candidate D
      const auto &[pvd_d, candidate_d] =
          make_committed_candidate(para_id,
                                   relay_parent_z_info.hash,
                                   relay_parent_z_info.number,
                                   {0x0d},
                                   {0x0e},
                                   relay_parent_z_info.number);
      const auto candidate_d_hash = hash(candidate_d);
      const auto candidate_d_entry =
          CandidateEntry::create(candidate_d_hash,
                                 candidate_d,
                                 pvd_d,
                                 CandidateState::Backed,
                                 hasher_)
              .value();

      ASSERT_TRUE(populate_chain_from_previous_storage(scope, storage)
                      .can_add_candidate_as_potential(candidate_d_entry)
                      .has_value());
      ASSERT_TRUE(storage.add_candidate_entry(candidate_d_entry).has_value());

      // Candidate F
      const auto &[pvd_f, candidate_f] =
          make_committed_candidate(para_id,
                                   relay_parent_z_info.hash,
                                   relay_parent_z_info.number,
                                   {0x0f},
                                   {0xf1},
                                   1000);
      const auto candidate_f_hash = hash(candidate_f);
      const auto candidate_f_entry =
          CandidateEntry::create(candidate_f_hash,
                                 candidate_f,
                                 pvd_f,
                                 CandidateState::Seconded,
                                 hasher_)
              .value();
      ASSERT_TRUE(populate_chain_from_previous_storage(scope, storage)
                      .can_add_candidate_as_potential(candidate_f_entry)
                      .has_value());
      ASSERT_TRUE(storage.add_candidate_entry(candidate_f_entry).has_value());

      // Candidate A1
      const auto &[pvd_a1, candidate_a1] =
          make_committed_candidate(para_id,
                                   relay_parent_x_info.hash,
                                   relay_parent_x_info.number,
                                   {0x0a},
                                   {0xb1},
                                   relay_parent_x_info.number);
      const auto candidate_a1_hash = hash(candidate_a1);
      const auto candidate_a1_entry =
          CandidateEntry::create(candidate_a1_hash,
                                 candidate_a1,
                                 pvd_a1,
                                 CandidateState::Backed,
                                 hasher_)
              .value();
      // Candidate A1 is created so that its hash is greater than the candidate
      // A hash.
      ASSERT_TRUE(FragmentChain::fork_selection_rule(candidate_a_hash,
                                                     candidate_a1_hash));
      ASSERT_EQ(populate_chain_from_previous_storage(scope, storage)
                    .can_add_candidate_as_potential(candidate_a1_entry)
                    .error(),
                FragmentChain::Error::FORK_CHOICE_RULE);
      ASSERT_TRUE(storage.add_candidate_entry(candidate_a1_entry).has_value());

      // Candidate B1.
      const auto &[pvd_b1, candidate_b1] =
          make_committed_candidate(para_id,
                                   relay_parent_x_info.hash,
                                   relay_parent_x_info.number,
                                   {0xb1},
                                   {0xc1},
                                   relay_parent_x_info.number);
      const auto candidate_b1_hash = hash(candidate_b1);
      const auto candidate_b1_entry =
          CandidateEntry::create(candidate_b1_hash,
                                 candidate_b1,
                                 pvd_b1,
                                 CandidateState::Seconded,
                                 hasher_)
              .value();
      ASSERT_TRUE(populate_chain_from_previous_storage(scope, storage)
                      .can_add_candidate_as_potential(candidate_b1_entry)
                      .has_value());
      ASSERT_TRUE(storage.add_candidate_entry(candidate_b1_entry).has_value());

      // Candidate C1.
      const auto &[pvd_c1, candidate_c1] =
          make_committed_candidate(para_id,
                                   relay_parent_x_info.hash,
                                   relay_parent_x_info.number,
                                   {0xc1},
                                   {0xd1},
                                   relay_parent_x_info.number);
      const auto candidate_c1_hash = hash(candidate_c1);
      const auto candidate_c1_entry =
          CandidateEntry::create(candidate_c1_hash,
                                 candidate_c1,
                                 pvd_c1,
                                 CandidateState::Backed,
                                 hasher_)
              .value();
      ASSERT_TRUE(populate_chain_from_previous_storage(scope, storage)
                      .can_add_candidate_as_potential(candidate_c1_entry)
                      .has_value());
      ASSERT_TRUE(storage.add_candidate_entry(candidate_c1_entry).has_value());

      // Candidate C2.
      const auto &[pvd_c2, candidate_c2] =
          make_committed_candidate(para_id,
                                   relay_parent_x_info.hash,
                                   relay_parent_x_info.number,
                                   {0xc1},
                                   {0xd2},
                                   relay_parent_x_info.number);
      const auto candidate_c2_hash = hash(candidate_c2);
      const auto candidate_c2_entry =
          CandidateEntry::create(candidate_c2_hash,
                                 candidate_c2,
                                 pvd_c2,
                                 CandidateState::Seconded,
                                 hasher_)
              .value();
      ASSERT_TRUE(populate_chain_from_previous_storage(scope, storage)
                      .can_add_candidate_as_potential(candidate_c2_entry)
                      .has_value());
      ASSERT_TRUE(storage.add_candidate_entry(candidate_c2_entry).has_value());

      // Candidate A2.
      const auto &[pvd_a2, candidate_a2] =
          make_committed_candidate(para_id,
                                   relay_parent_x_info.hash,
                                   relay_parent_x_info.number,
                                   {0x0a},
                                   {0xb3},
                                   relay_parent_x_info.number);
      const auto candidate_a2_hash = hash(candidate_a2);
      const auto candidate_a2_entry =
          CandidateEntry::create(candidate_a2_hash,
                                 candidate_a2,
                                 pvd_a2,
                                 CandidateState::Seconded,
                                 hasher_)
              .value();
      // Candidate A2 is created so that its hash is greater than the candidate
      // A hash.
      ASSERT_TRUE(FragmentChain::fork_selection_rule(candidate_a2_hash,
                                                     candidate_a_hash));

      ASSERT_TRUE(populate_chain_from_previous_storage(scope, storage)
                      .can_add_candidate_as_potential(candidate_a2_entry)
                      .has_value());
      ASSERT_TRUE(storage.add_candidate_entry(candidate_a2_entry).has_value());

      // Candidate B2.
      const auto &[pvd_b2, candidate_b2] =
          make_committed_candidate(para_id,
                                   relay_parent_y_info.hash,
                                   relay_parent_y_info.number,
                                   {0xb3},
                                   {0xb4},
                                   relay_parent_y_info.number);
      const auto candidate_b2_hash = hash(candidate_b2);
      const auto candidate_b2_entry =
          CandidateEntry::create(candidate_b2_hash,
                                 candidate_b2,
                                 pvd_b2,
                                 CandidateState::Backed,
                                 hasher_)
              .value();
      ASSERT_TRUE(populate_chain_from_previous_storage(scope, storage)
                      .can_add_candidate_as_potential(candidate_b2_entry)
                      .has_value());
      ASSERT_TRUE(storage.add_candidate_entry(candidate_b2_entry).has_value());

      {
        const auto chain = populate_chain_from_previous_storage(scope, storage);
        {
          const Vec<CandidateHash> ref = {
              candidate_a_hash, candidate_b_hash, candidate_c_hash};
          ASSERT_EQ(chain.best_chain_vec(), ref);
        }
        {
          const HashSet<CandidateHash> ref = {candidate_d_hash,
                                              candidate_f_hash,
                                              candidate_a2_hash,
                                              candidate_b2_hash};
          ASSERT_EQ(get_unconnected(chain), ref);
        }

        // Cannot add as potential an already present candidate (whether it's in
        // the best chain or in unconnected storage)
        ASSERT_EQ(
            chain.can_add_candidate_as_potential(candidate_a_entry).error(),
            FragmentChain::Error::CANDIDATE_ALREADY_KNOWN);
        ASSERT_EQ(
            chain.can_add_candidate_as_potential(candidate_f_entry).error(),
            FragmentChain::Error::CANDIDATE_ALREADY_KNOWN);

        // Simulate a best chain reorg by backing a2.
        {
          FragmentChain chain_2 = chain;
          chain_2.candidate_backed(candidate_a2_hash);
          {
            const Vec<CandidateHash> ref = {candidate_a2_hash,
                                            candidate_b2_hash};
            ASSERT_EQ(chain_2.best_chain_vec(), ref);
          }
          {
            // F is kept as it was truly unconnected. The rest will be trimmed.
            const HashSet<CandidateHash> ref = {candidate_f_hash};
            ASSERT_EQ(get_unconnected(chain_2), ref);
          }
          // A and A1 will never have potential again.
          ASSERT_EQ(chain_2.can_add_candidate_as_potential(candidate_a1_entry)
                        .error(),
                    FragmentChain::Error::FORK_CHOICE_RULE);
          ASSERT_EQ(
              chain_2.can_add_candidate_as_potential(candidate_a_entry).error(),
              FragmentChain::Error::FORK_CHOICE_RULE);
        }
      }
      // Candidate F has an invalid hrmp watermark. however, it was not checked
      // beforehand as we don't have its parent yet. Add its parent now. This
      // will not impact anything as E is not yet part of the best chain.

      const auto &[pvd_e, candidate_e] =
          make_committed_candidate(para_id,
                                   relay_parent_z_info.hash,
                                   relay_parent_z_info.number,
                                   {0x0e},
                                   {0x0f},
                                   relay_parent_z_info.number);
      const auto candidate_e_hash = hash(candidate_e);
      ASSERT_TRUE(storage
                      .add_candidate_entry(
                          CandidateEntry::create(candidate_e_hash,
                                                 candidate_e,
                                                 pvd_e,
                                                 CandidateState::Seconded,
                                                 hasher_)
                              .value())
                      .has_value());

      {
        const auto chain = populate_chain_from_previous_storage(scope, storage);
        {
          const Vec<CandidateHash> ref = {
              candidate_a_hash, candidate_b_hash, candidate_c_hash};
          ASSERT_EQ(chain.best_chain_vec(), ref);
        }
        {
          const HashSet<CandidateHash> ref = {candidate_d_hash,
                                              candidate_f_hash,
                                              candidate_a2_hash,
                                              candidate_b2_hash,
                                              candidate_e_hash};
          ASSERT_EQ(get_unconnected(chain), ref);
        }
      }

      // Simulate the fact that candidates A, B, C are now pending availability.
      EXPECT_OUTCOME_TRUE(
          scope2,
          Scope::with_ancestors(relay_parent_z_info,
                                base_constraints,
                                {PendingAvailability{
                                     .candidate_hash = candidate_a_hash,
                                     .relay_parent = relay_parent_x_info,
                                 },
                                 PendingAvailability{
                                     .candidate_hash = candidate_b_hash,
                                     .relay_parent = relay_parent_y_info,
                                 },
                                 PendingAvailability{
                                     .candidate_hash = candidate_c_hash,
                                     .relay_parent = relay_parent_z_info,
                                 }},
                                2,
                                ancestors));

      // A2 and B2 will now be trimmed
      const auto chain = populate_chain_from_previous_storage(scope2, storage);
      {
        const Vec<CandidateHash> ref = {
            candidate_a_hash, candidate_b_hash, candidate_c_hash};
        ASSERT_EQ(chain.best_chain_vec(), ref);
      }
      {
        const HashSet<CandidateHash> ref = {
            candidate_d_hash, candidate_f_hash, candidate_e_hash};
        ASSERT_EQ(get_unconnected(chain), ref);
      }

      // Cannot add as potential an already pending availability candidate
      ASSERT_EQ(chain.can_add_candidate_as_potential(candidate_a_entry).error(),
                FragmentChain::Error::CANDIDATE_ALREADY_KNOWN);

      // Simulate the fact that candidates A, B and C have been included.
      EXPECT_OUTCOME_TRUE(
          scope3,
          Scope::with_ancestors(relay_parent_z_info,
                                make_constraints(0, {0}, {0x0d}),
                                {},
                                2,
                                ancestors));

      FragmentChain prev_chain = chain;
      FragmentChain chain_new =
          FragmentChain::init(hasher_, scope3, CandidateStorage{});
      chain_new.populate_from_previous(prev_chain);
      {
        const Vec<CandidateHash> ref = {candidate_d_hash};
        ASSERT_EQ(chain_new.best_chain_vec(), ref);
      }
      {
        const HashSet<CandidateHash> ref = {candidate_e_hash, candidate_f_hash};
        ASSERT_EQ(get_unconnected(chain_new), ref);
      }

      // Mark E as backed. F will be dropped for invalid watermark. No other
      // unconnected candidates.
      chain_new.candidate_backed(candidate_e_hash);
      {
        const Vec<CandidateHash> ref = {candidate_d_hash, candidate_e_hash};
        ASSERT_EQ(chain_new.best_chain_vec(), ref);
      }
      ASSERT_EQ(chain_new.unconnected_len(), 0);

      ASSERT_EQ(
          chain_new.can_add_candidate_as_potential(candidate_f_entry).error(),
          FragmentChain::Error::CHECK_AGAINST_CONSTRAINTS);
    }
  }
}

TEST_F(FragmentChainTest,
       test_find_ancestor_path_and_find_backable_chain_empty_best_chain) {
  const auto relay_parent = fromNumber(1);
  HeadData required_parent = {0xff};
  size_t max_depth = 10;

  // Empty chain
  const auto base_constraints = make_constraints(0, {0}, required_parent);

  const RelayChainBlockInfo relay_parent_info{
      .hash = relay_parent,
      .number = 0,
      .storage_root = fromNumber(0),
  };

  EXPECT_OUTCOME_TRUE(
      scope,
      Scope::with_ancestors(
          relay_parent_info, base_constraints, {}, max_depth, {}));
  const auto chain = FragmentChain::init(hasher_, scope, CandidateStorage{});
  ASSERT_EQ(chain.best_chain_len(), 0);

  Vec<std::pair<CandidateHash, Hash>> ref;
  ASSERT_EQ(chain.find_ancestor_path({}), 0);
  ASSERT_EQ(chain.find_backable_chain({}, 2), ref);

  // Invalid candidate.
  Ancestors ancestors = {CandidateHash{}};
  ASSERT_EQ(chain.find_ancestor_path(ancestors), 0);
  ASSERT_EQ(chain.find_backable_chain(ancestors, 2), ref);
}

TEST_F(FragmentChainTest, test_find_ancestor_path_and_find_backable_chain) {
  const ParachainId para_id{5};
  const auto relay_parent = fromNumber(1);
  HeadData required_parent = {0xff};
  size_t max_depth = 5;
  BlockNumber relay_parent_number = 0;
  auto relay_parent_storage_root = fromNumber(0);

  Vec<std::pair<crypto::Hashed<runtime::PersistedValidationData,
                               32,
                               crypto::Blake2b_StreamHasher<32>>,
                network::CommittedCandidateReceipt>>
      candidates;

  // Candidate 0
  candidates.emplace_back(make_committed_candidate(
      para_id, relay_parent, 0, required_parent, {0}, 0));

  // Candidates 1..=5
  for (uint8_t index = 1; index <= 5; ++index) {
    candidates.emplace_back(make_committed_candidate(
        para_id, relay_parent, 0, {uint8_t(index - 1)}, {index}, 0));
  }

  CandidateStorage storage;
  for (const auto &[pvd, candidate] : candidates) {
    ASSERT_TRUE(
        storage
            .add_candidate_entry(CandidateEntry::create_seconded(
                                     hash(candidate), candidate, pvd, hasher_)
                                     .value())
            .has_value());
  }

  Vec<CandidateHash> candidate_hashes;
  for (const auto &[_, candidate] : candidates) {
    candidate_hashes.emplace_back(hash(candidate));
  }

  auto hashes = [&](size_t from, size_t to) {
    Vec<std::pair<CandidateHash, Hash>> result;
    for (size_t ix = from; ix < to; ++ix) {
      result.emplace_back(candidate_hashes[ix], relay_parent);
    }
    return result;
  };

  const RelayChainBlockInfo relay_parent_info{
      .hash = relay_parent,
      .number = relay_parent_number,
      .storage_root = relay_parent_storage_root,
  };

  const auto base_constraints = make_constraints(0, {0}, required_parent);
  EXPECT_OUTCOME_TRUE(
      scope,
      Scope::with_ancestors(
          relay_parent_info, base_constraints, {}, max_depth, {}));
  auto chain = populate_chain_from_previous_storage(scope, storage);

  // For now, candidates are only seconded, not backed. So the best chain is
  // empty and no candidate will be returned.
  ASSERT_EQ(candidate_hashes.size(), 6);
  ASSERT_EQ(chain.best_chain_len(), 0);
  ASSERT_EQ(chain.unconnected_len(), 6);

  for (size_t count = 0; count < 10; ++count) {
    ASSERT_EQ(chain.find_backable_chain(Ancestors{}, count).size(), 0);
  }

  // Do tests with only a couple of candidates being backed.
  {
    auto chain_new = chain;
    chain_new.candidate_backed(candidate_hashes[5]);
    ASSERT_EQ(chain_new.unconnected_len(), 6);
    for (size_t count = 0; count < 10; ++count) {
      ASSERT_EQ(chain_new.find_backable_chain(Ancestors{}, count).size(), 0);
    }

    chain_new.candidate_backed(candidate_hashes[3]);
    ASSERT_EQ(chain_new.unconnected_len(), 6);
    chain_new.candidate_backed(candidate_hashes[4]);
    ASSERT_EQ(chain_new.unconnected_len(), 6);
    for (size_t count = 0; count < 10; ++count) {
      ASSERT_EQ(chain_new.find_backable_chain(Ancestors{}, count).size(), 0);
    }

    chain_new.candidate_backed(candidate_hashes[1]);
    ASSERT_EQ(chain_new.unconnected_len(), 6);
    for (size_t count = 0; count < 10; ++count) {
      ASSERT_EQ(chain_new.find_backable_chain(Ancestors{}, count).size(), 0);
    }

    chain_new.candidate_backed(candidate_hashes[0]);
    ASSERT_EQ(chain_new.unconnected_len(), 4);
    ASSERT_EQ(chain_new.find_backable_chain(Ancestors{}, 1), hashes(0, 1));
    for (size_t count = 2; count < 10; ++count) {
      ASSERT_EQ(chain_new.find_backable_chain(Ancestors{}, count),
                hashes(0, 2));
    }

    // Now back the missing piece.
    chain_new.candidate_backed(candidate_hashes[2]);
    ASSERT_EQ(chain_new.unconnected_len(), 0);
    ASSERT_EQ(chain_new.best_chain_len(), 6);

    for (size_t count = 0; count < 10; ++count) {
      ASSERT_EQ(chain_new.find_backable_chain(Ancestors{}, count),
                hashes(0, std::min(count, size_t(6))));
    }
  }

  // Now back all candidates. Back them in a random order. The result should
  // always be the same.
  auto candidates_shuffled = candidate_hashes;
  std::default_random_engine random_;
  std::shuffle(candidates_shuffled.begin(), candidates_shuffled.end(), random_);
  for (const auto &candidate : candidates_shuffled) {
    chain.candidate_backed(candidate);
    storage.mark_backed(candidate);
  }

  // No ancestors supplied.
  ASSERT_EQ(chain.find_ancestor_path(Ancestors{}), 0);
  ASSERT_EQ(chain.find_backable_chain(Ancestors{}, 0), hashes(0, 0));
  ASSERT_EQ(chain.find_backable_chain(Ancestors{}, 1), hashes(0, 1));
  ASSERT_EQ(chain.find_backable_chain(Ancestors{}, 2), hashes(0, 2));
  ASSERT_EQ(chain.find_backable_chain(Ancestors{}, 5), hashes(0, 5));

  for (size_t count = 6; count < 10; ++count) {
    ASSERT_EQ(chain.find_backable_chain(Ancestors{}, count), hashes(0, 6));
  }

  ASSERT_EQ(chain.find_backable_chain(Ancestors{}, 7), hashes(0, 6));
  ASSERT_EQ(chain.find_backable_chain(Ancestors{}, 10), hashes(0, 6));

  // Ancestor which is not part of the chain. Will be ignored.
  {
    Ancestors ancestors = {CandidateHash{}};
    ASSERT_EQ(chain.find_ancestor_path(ancestors), 0);
    ASSERT_EQ(chain.find_backable_chain(ancestors, 4), hashes(0, 4));
  }

  {
    Ancestors ancestors = {candidate_hashes[1], CandidateHash{}};
    ASSERT_EQ(chain.find_ancestor_path(ancestors), 0);
    ASSERT_EQ(chain.find_backable_chain(ancestors, 4), hashes(0, 4));
  }

  {
    Ancestors ancestors = {candidate_hashes[0], CandidateHash{}};
    ASSERT_EQ(chain.find_ancestor_path(ancestors), 1);
    ASSERT_EQ(chain.find_backable_chain(ancestors, 4), hashes(1, 5));
  }

  {
    // Ancestors which are part of the chain but don't form a path from root.
    // Will be ignored.
    Ancestors ancestors = {candidate_hashes[1], candidate_hashes[2]};
    ASSERT_EQ(chain.find_ancestor_path(ancestors), 0);
    ASSERT_EQ(chain.find_backable_chain(ancestors, 4), hashes(0, 4));
  }

  {
    // Valid ancestors.
    Ancestors ancestors = {
        candidate_hashes[2], candidate_hashes[0], candidate_hashes[1]};
    ASSERT_EQ(chain.find_ancestor_path(ancestors), 3);
    ASSERT_EQ(chain.find_backable_chain(ancestors, 2), hashes(3, 5));
    for (size_t count = 3; count < 10; ++count) {
      ASSERT_EQ(chain.find_backable_chain(ancestors, count), hashes(3, 6));
    }
  }

  {
    // Valid ancestors with candidates which have been omitted due to timeouts
    Ancestors ancestors = {candidate_hashes[0], candidate_hashes[2]};
    ASSERT_EQ(chain.find_ancestor_path(ancestors), 1);
    ASSERT_EQ(chain.find_backable_chain(ancestors, 3), hashes(1, 4));
    ASSERT_EQ(chain.find_backable_chain(ancestors, 4), hashes(1, 5));
    for (size_t count = 5; count < 10; ++count) {
      ASSERT_EQ(chain.find_backable_chain(ancestors, count), hashes(1, 6));
    }
  }

  {
    Ancestors ancestors = {
        candidate_hashes[0], candidate_hashes[1], candidate_hashes[3]};
    ASSERT_EQ(chain.find_ancestor_path(ancestors), 2);
    ASSERT_EQ(chain.find_backable_chain(ancestors, 4), hashes(2, 6));

    // Requested count is 0.
    ASSERT_EQ(chain.find_backable_chain(ancestors, 0), hashes(0, 0));
  }

  // Stop when we've found a candidate which is pending availability
  {
    EXPECT_OUTCOME_TRUE(
        scope2,
        Scope::with_ancestors(relay_parent_info,
                              base_constraints,
                              {PendingAvailability{
                                  .candidate_hash = candidate_hashes[3],
                                  .relay_parent = relay_parent_info,
                              }},
                              max_depth,
                              {}));

    auto chain = populate_chain_from_previous_storage(scope2, storage);
    Ancestors ancestors = {candidate_hashes[0], candidate_hashes[1]};
    ASSERT_EQ(chain.find_backable_chain(ancestors, 3), hashes(2, 3));
  }
}
