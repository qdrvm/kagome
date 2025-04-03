/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "parachain/validator/prospective_parachains/fragment_chain.hpp"
#include "core/parachain/parachain_test_harness.hpp"

#include <random>

using namespace kagome::parachain::fragment;

class FragmentChainTest : public ProspectiveParachainsTestHarness {
  void SetUp() override {
    ProspectiveParachainsTestHarness::SetUp();
  }

  void TearDown() override {
    ProspectiveParachainsTestHarness::TearDown();
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
  // Create some base constraints for an empty chain
  const auto base_constraints = make_constraints(0, {0}, {0x0a});

  // Create a scope with empty ancestors
  ASSERT_OUTCOME_SUCCESS(scope,
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

  // Initialize a chain with empty storage
  auto chain = FragmentChain::init(hasher_, scope, CandidateStorage{});
  // Verify the chain is empty
  ASSERT_EQ(chain.best_chain_len(), 0);
  ASSERT_EQ(chain.unconnected_len(), 0);

  // Create a new chain and populate it from the previous empty chain
  auto new_chain = FragmentChain::init(hasher_, scope, CandidateStorage{});
  new_chain.populate_from_previous(chain);
  // Verify the new chain is also empty
  ASSERT_EQ(new_chain.best_chain_len(), 0);
  ASSERT_EQ(new_chain.unconnected_len(), 0);
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
  {
    for (const auto &wrong_constraints :
         {make_constraints(
              relay_parent_x_info.number, {relay_parent_x_info.number}, {0x0e}),
          make_constraints(relay_parent_y_info.number, {0}, {0x0a})}) {
      ASSERT_OUTCOME_SUCCESS(
          scope_wrong_constraints,
          Scope::with_ancestors(
              relay_parent_z_info, wrong_constraints, {}, 5, ancestors));
      auto chain = populate_chain_from_previous_storage(scope_wrong_constraints,
                                                        storage);
      ASSERT_TRUE(chain.best_chain_vec().empty());

      if (wrong_constraints.min_relay_parent_number
          == relay_parent_y_info.number) {
        ASSERT_EQ(chain.unconnected_len(), 0);
        ASSERT_EQ(
            chain.can_add_candidate_as_potential(candidate_a_entry).error(),
            FragmentChainError::RELAY_PARENT_NOT_IN_SCOPE);
        // However, if taken independently, both B and C still have potential,
        // since we don't know that A doesn't.
        ASSERT_TRUE(chain.can_add_candidate_as_potential(candidate_b_entry)
                        .has_value());
        ASSERT_TRUE(chain.can_add_candidate_as_potential(candidate_c_entry)
                        .has_value());
      } else {
        const HashSet<CandidateHash> unconnected_ref = {
            candidate_a_hash, candidate_b_hash, candidate_c_hash};
        ASSERT_EQ(get_unconnected(chain), unconnected_ref);
      }
    }
  }

  // Various depths
  {
    // Depth is 0, doesn't allow any candidate, but the others will be kept as
    // potential.
    ASSERT_OUTCOME_SUCCESS(
        scope_depth_0,
        Scope::with_ancestors(
            relay_parent_z_info, base_constraints, {}, 0, ancestors));
    {
      const auto chain =
          FragmentChain::init(hasher_, scope_depth_0, CandidateStorage{});
      ASSERT_TRUE(
          chain.can_add_candidate_as_potential(candidate_a_entry).has_value());
      ASSERT_TRUE(
          chain.can_add_candidate_as_potential(candidate_b_entry).has_value());
      ASSERT_TRUE(
          chain.can_add_candidate_as_potential(candidate_c_entry).has_value());
    }

    {
      const auto chain =
          populate_chain_from_previous_storage(scope_depth_0, storage);
      ASSERT_TRUE(chain.best_chain_vec().empty());
      const HashSet<CandidateHash> unconnected_ref = {
          candidate_a_hash, candidate_b_hash, candidate_c_hash};
      ASSERT_EQ(get_unconnected(chain), unconnected_ref);
    }

    // depth is 1, allows one candidate, but the others will be kept as
    // potential.
    ASSERT_OUTCOME_SUCCESS(
        scope_depth_1,
        Scope::with_ancestors(
            relay_parent_z_info, base_constraints, {}, 1, ancestors));
    {
      const auto chain =
          FragmentChain::init(hasher_, scope_depth_1, CandidateStorage{});
      ASSERT_TRUE(
          chain.can_add_candidate_as_potential(candidate_a_entry).has_value());
      ASSERT_TRUE(
          chain.can_add_candidate_as_potential(candidate_b_entry).has_value());
      ASSERT_TRUE(
          chain.can_add_candidate_as_potential(candidate_c_entry).has_value());
    }
    {
      const auto chain =
          populate_chain_from_previous_storage(scope_depth_1, storage);
      const Vec<CandidateHash> best_chain_ref = {candidate_a_hash};
      ASSERT_EQ(chain.best_chain_vec(), best_chain_ref);
      const HashSet<CandidateHash> unconnected_ref = {candidate_b_hash,
                                                      candidate_c_hash};
      ASSERT_EQ(get_unconnected(chain), unconnected_ref);
    }
  }

  // depth is 2, allows two candidates
  {
    ASSERT_OUTCOME_SUCCESS(
        scope_depth_2,
        Scope::with_ancestors(
            relay_parent_z_info, base_constraints, {}, 2, ancestors));
    {
      const auto chain =
          FragmentChain::init(hasher_, scope_depth_2, CandidateStorage{});
      ASSERT_TRUE(
          chain.can_add_candidate_as_potential(candidate_a_entry).has_value());
      ASSERT_TRUE(
          chain.can_add_candidate_as_potential(candidate_b_entry).has_value());
      ASSERT_TRUE(
          chain.can_add_candidate_as_potential(candidate_c_entry).has_value());
    }
    {
      const auto chain =
          populate_chain_from_previous_storage(scope_depth_2, storage);
      const Vec<CandidateHash> best_chain_ref = {candidate_a_hash,
                                                 candidate_b_hash};
      ASSERT_EQ(chain.best_chain_vec(), best_chain_ref);

      const HashSet<CandidateHash> unconnected_ref = {candidate_c_hash};
      ASSERT_EQ(get_unconnected(chain), unconnected_ref);
    }
  }

  // depth is at least 3, allows all three candidates
  for (size_t depth = 3; depth < 6; ++depth) {
    ASSERT_OUTCOME_SUCCESS(
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
      const Vec<CandidateHash> best_chain_ref = {
          candidate_a_hash, candidate_b_hash, candidate_c_hash};
      ASSERT_EQ(chain.best_chain_vec(), best_chain_ref);
      ASSERT_EQ(chain.unconnected_len(), 0);
    }
  }

  // Relay parents out of scope
  {
    // Candidate A has relay parent out of scope. Candidates B and C will also
    // be deleted since they form a chain with A.
    Vec<RelayChainBlockInfo> ancestors_without_x = {relay_parent_y_info};
    ASSERT_OUTCOME_SUCCESS(
        scope_without_x,
        Scope::with_ancestors(
            relay_parent_z_info, base_constraints, {}, 5, ancestors_without_x));

    const auto chain =
        populate_chain_from_previous_storage(scope_without_x, storage);
    ASSERT_TRUE(chain.best_chain_vec().empty());
    ASSERT_EQ(chain.unconnected_len(), 0);

    ASSERT_EQ(chain.can_add_candidate_as_potential(candidate_a_entry).error(),
              FragmentChainError::RELAY_PARENT_NOT_IN_SCOPE);
    // However, if taken independently, both B and C still have potential, since
    // we don't know that A doesn't.
    ASSERT_TRUE(
        chain.can_add_candidate_as_potential(candidate_b_entry).has_value());
    ASSERT_TRUE(
        chain.can_add_candidate_as_potential(candidate_c_entry).has_value());

    // Candidates A and B have relay parents out of scope. Candidate C will also
    // be deleted since it forms a chain with A and B.
    ASSERT_OUTCOME_SUCCESS(
        scope_empty,
        Scope::with_ancestors(
            relay_parent_z_info, base_constraints, {}, 5, {}));

    const auto chain2 =
        populate_chain_from_previous_storage(scope_empty, storage);
    ASSERT_TRUE(chain2.best_chain_vec().empty());
    ASSERT_EQ(chain2.unconnected_len(), 0);

    ASSERT_EQ(chain2.can_add_candidate_as_potential(candidate_a_entry).error(),
              FragmentChainError::RELAY_PARENT_NOT_IN_SCOPE);
    ASSERT_EQ(chain2.can_add_candidate_as_potential(candidate_b_entry).error(),
              FragmentChainError::RELAY_PARENT_NOT_IN_SCOPE);
    // However, if taken independently, C still has potential, since we
    // don't know that A and B don't
    ASSERT_TRUE(
        chain2.can_add_candidate_as_potential(candidate_c_entry).has_value());
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
    const auto wrong_candidate_c_hash =
        network::candidateHash(*hasher_, wrong_candidate_c);
    const auto wrong_candidate_c_entry =
        CandidateEntry::create(wrong_candidate_c_hash,
                               wrong_candidate_c,
                               wrong_pvd_c,
                               CandidateState::Backed,
                               hasher_)
            .value();
    ASSERT_TRUE(modified_storage.add_candidate_entry(wrong_candidate_c_entry)
                    .has_value());
    ASSERT_OUTCOME_SUCCESS(
        scope_cycle,
        Scope::with_ancestors(
            relay_parent_z_info, base_constraints, {}, 5, ancestors));

    const auto chain =
        populate_chain_from_previous_storage(scope_cycle, modified_storage);
    Vec<CandidateHash> expected_chain = {candidate_a_hash, candidate_b_hash};
    ASSERT_EQ(chain.best_chain_vec(), expected_chain);
    ASSERT_EQ(chain.unconnected_len(), 0);

    ASSERT_EQ(
        chain.can_add_candidate_as_potential(wrong_candidate_c_entry).error(),
        FragmentChainError::CYCLE);
    // However, if taken independently, C still has potential, since we don't
    // know A and B.
    const auto chain2 =
        FragmentChain::init(hasher_, scope_cycle, CandidateStorage{});
    ASSERT_TRUE(chain2.can_add_candidate_as_potential(wrong_candidate_c_entry)
                    .has_value());
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
    const auto wrong_candidate_c_hash =
        network::candidateHash(*hasher_, wrong_candidate_c);
    const auto wrong_candidate_c_entry =
        CandidateEntry::create(wrong_candidate_c_hash,
                               wrong_candidate_c,
                               wrong_pvd_c,
                               CandidateState::Backed,
                               hasher_)
            .value();
    ASSERT_TRUE(modified_storage.add_candidate_entry(wrong_candidate_c_entry)
                    .has_value());
    ASSERT_OUTCOME_SUCCESS(
        scope_backwards,
        Scope::with_ancestors(
            relay_parent_z_info, base_constraints, {}, 5, ancestors));

    const auto chain =
        populate_chain_from_previous_storage(scope_backwards, modified_storage);
    Vec<CandidateHash> expected_chain = {candidate_a_hash, candidate_b_hash};
    ASSERT_EQ(chain.best_chain_vec(), expected_chain);
    ASSERT_EQ(chain.unconnected_len(), 0);
    ASSERT_EQ(
        chain.can_add_candidate_as_potential(wrong_candidate_c_entry).error(),
        FragmentChainError::RELAY_PARENT_MOVED_BACKWARDS);
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
    const auto unconnected_candidate_c_entry =
        CandidateEntry::create(unconnected_candidate_c_hash,
                               unconnected_candidate_c,
                               unconnected_pvd_c,
                               CandidateState::Backed,
                               hasher_)
            .value();
    ASSERT_TRUE(
        modified_storage.add_candidate_entry(unconnected_candidate_c_entry)
            .has_value());
    ASSERT_OUTCOME_SUCCESS(
        scope_unconnected,
        Scope::with_ancestors(
            relay_parent_z_info, base_constraints, {}, 5, ancestors));

    const auto chain =
        FragmentChain::init(hasher_, scope_unconnected, CandidateStorage{});
    ASSERT_TRUE(
        chain.can_add_candidate_as_potential(unconnected_candidate_c_entry)
            .has_value());

    const auto chain2 = populate_chain_from_previous_storage(scope_unconnected,
                                                             modified_storage);
    Vec<CandidateHash> expected_chain = {candidate_a_hash, candidate_b_hash};
    ASSERT_EQ(chain2.best_chain_vec(), expected_chain);
    ASSERT_EQ(get_unconnected(chain2),
              HashSet<CandidateHash>{unconnected_candidate_c_hash});
  }

  // Candidate A is a pending availability candidate and Candidate C is an
  // unconnected candidate, C's relay parent is not allowed to move backwards
  // from A's relay parent because we're sure A will not get removed in the
  // future, as it's already on-chain (unless it times out availability, a case
  // for which we don't care to optimise for)
  {
    fragment::CandidateStorage modified_storage = storage;
    modified_storage.remove_candidate(candidate_a_hash, hasher_);
    // Also remove candidate_c to prevent it from being included in the best
    // chain
    modified_storage.remove_candidate(candidate_c_hash, hasher_);
    const auto &[modified_pvd_a, modified_candidate_a] =
        make_committed_candidate(para_id,
                                 relay_parent_y_info.hash,
                                 relay_parent_y_info.number,
                                 {0x0a},
                                 {0x0b},
                                 relay_parent_y_info.number);
    const auto modified_candidate_a_hash =
        network::candidateHash(*hasher_, modified_candidate_a);
    const auto modified_candidate_a_entry =
        CandidateEntry::create(modified_candidate_a_hash,
                               modified_candidate_a,
                               modified_pvd_a,
                               CandidateState::Backed,
                               hasher_)
            .value();
    ASSERT_TRUE(modified_storage.add_candidate_entry(modified_candidate_a_entry)
                    .has_value());

    ASSERT_OUTCOME_SUCCESS(
        scope_candidate_a_pending,
        Scope::with_ancestors(relay_parent_z_info,
                              base_constraints,
                              {PendingAvailability{
                                  .candidate_hash = modified_candidate_a_hash,
                                  .relay_parent = relay_parent_y_info,
                              }},
                              4,
                              ancestors));

    const auto chain = populate_chain_from_previous_storage(
        scope_candidate_a_pending, modified_storage);
    Vec<CandidateHash> expected_chain = {modified_candidate_a_hash,
                                         candidate_b_hash};
    ASSERT_EQ(chain.best_chain_vec(), expected_chain);
    ASSERT_EQ(chain.unconnected_len(), 0);

    // From the original implementation, we need to create an
    // unconnected_candidate_c if it doesn't exist
    const auto &[unconnected_pvd_c, unconnected_candidate_c] =
        make_committed_candidate(para_id,
                                 relay_parent_x_info.hash,
                                 relay_parent_x_info.number,
                                 {0x0d},
                                 {0x0e},
                                 0);
    const auto unconnected_candidate_c_hash =
        network::candidateHash(*hasher_, unconnected_candidate_c);
    const auto unconnected_candidate_c_entry =
        CandidateEntry::create(unconnected_candidate_c_hash,
                               unconnected_candidate_c,
                               unconnected_pvd_c,
                               CandidateState::Backed,
                               hasher_)
            .value();

    ASSERT_EQ(
        chain.can_add_candidate_as_potential(unconnected_candidate_c_entry)
            .error(),
        FragmentChainError::
            RELAY_PARENT_PRECEDES_CANDIDATE_PENDING_AVAILABILITY);
  }

  // Not allowed to fork from a candidate pending availability
  {
    fragment::CandidateStorage modified_storage = storage;
    modified_storage.remove_candidate(candidate_a_hash, hasher_);
    // Also remove candidate_c to prevent it from being included in the best
    // chain
    modified_storage.remove_candidate(candidate_c_hash, hasher_);
    const auto &[modified_pvd_a, modified_candidate_a] =
        make_committed_candidate(para_id,
                                 relay_parent_y_info.hash,
                                 relay_parent_y_info.number,
                                 {0x0a},
                                 {0x0b},
                                 relay_parent_y_info.number);
    const auto modified_candidate_a_hash =
        network::candidateHash(*hasher_, modified_candidate_a);
    const auto modified_candidate_a_entry =
        CandidateEntry::create(modified_candidate_a_hash,
                               modified_candidate_a,
                               modified_pvd_a,
                               CandidateState::Backed,
                               hasher_)
            .value();
    ASSERT_TRUE(modified_storage.add_candidate_entry(modified_candidate_a_entry)
                    .has_value());

    const auto &[wrong_pvd_c, wrong_candidate_c] =
        make_committed_candidate(para_id,
                                 relay_parent_y_info.hash,
                                 relay_parent_y_info.number,
                                 {0x0a},
                                 {0x0b2},
                                 0);
    const auto wrong_candidate_c_hash =
        network::candidateHash(*hasher_, wrong_candidate_c);
    const auto wrong_candidate_c_entry =
        CandidateEntry::create(wrong_candidate_c_hash,
                               wrong_candidate_c,
                               wrong_pvd_c,
                               CandidateState::Backed,
                               hasher_)
            .value();
    ASSERT_TRUE(modified_storage.add_candidate_entry(wrong_candidate_c_entry)
                    .has_value());

    // Does not even matter if the fork selection rule would have picked up the
    // new candidate, as the other is already pending availability.
    ASSERT_TRUE(FragmentChain::fork_selection_rule(wrong_candidate_c_hash,
                                                   modified_candidate_a_hash));

    ASSERT_OUTCOME_SUCCESS(
        scope_fork_pending,
        Scope::with_ancestors(relay_parent_z_info,
                              base_constraints,
                              {PendingAvailability{
                                  .candidate_hash = modified_candidate_a_hash,
                                  .relay_parent = relay_parent_y_info,
                              }},
                              4,
                              ancestors));

    const auto chain = populate_chain_from_previous_storage(scope_fork_pending,
                                                            modified_storage);
    Vec<CandidateHash> expected_chain = {modified_candidate_a_hash,
                                         candidate_b_hash};
    ASSERT_EQ(chain.best_chain_vec(), expected_chain);
    ASSERT_EQ(chain.unconnected_len(), 0);
    ASSERT_EQ(
        chain.can_add_candidate_as_potential(wrong_candidate_c_entry).error(),
        FragmentChainError::FORK_WITH_CANDIDATE_PENDING_AVAILABILITY);
  }

  // Test with candidates pending availability
  {
    // Valid options
    for (const auto &pending : std::vector<Vec<PendingAvailability>>{
             {{PendingAvailability{
                 .candidate_hash = candidate_a_hash,
                 .relay_parent = relay_parent_x_info,
             }}},
             {{PendingAvailability{
                   .candidate_hash = candidate_a_hash,
                   .relay_parent = relay_parent_x_info,
               },
               PendingAvailability{
                   .candidate_hash = candidate_b_hash,
                   .relay_parent = relay_parent_y_info,
               }}},
             {{PendingAvailability{
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
               }}}}) {
      ASSERT_OUTCOME_SUCCESS(
          scope_pending,
          Scope::with_ancestors(
              relay_parent_z_info, base_constraints, pending, 3, ancestors));
      const auto chain =
          populate_chain_from_previous_storage(scope_pending, storage);
      Vec<CandidateHash> expected_chain = {
          candidate_a_hash, candidate_b_hash, candidate_c_hash};
      ASSERT_EQ(chain.best_chain_vec(), expected_chain);
      ASSERT_EQ(chain.unconnected_len(), 0);
    }

    // Relay parents of pending availability candidates can be out of scope
    // Relay parent of candidate A is out of scope.
    Vec<RelayChainBlockInfo> ancestors_without_x = {relay_parent_y_info};
    ASSERT_OUTCOME_SUCCESS(
        scope_pending_ancestors_without_x,
        Scope::with_ancestors(relay_parent_z_info,
                              base_constraints,
                              {PendingAvailability{
                                  .candidate_hash = candidate_a_hash,
                                  .relay_parent = relay_parent_x_info,
                              }},
                              4,
                              ancestors_without_x));
    const auto chain = populate_chain_from_previous_storage(
        scope_pending_ancestors_without_x, storage);
    Vec<CandidateHash> expected_chain = {
        candidate_a_hash, candidate_b_hash, candidate_c_hash};
    ASSERT_EQ(chain.best_chain_vec(), expected_chain);
    ASSERT_EQ(chain.unconnected_len(), 0);

    // Even relay parents of pending availability candidates which are out of
    // scope cannot move backwards.
    ASSERT_OUTCOME_SUCCESS(
        scope_pending_move_backwards,
        Scope::with_ancestors(
            relay_parent_z_info,
            base_constraints,
            {{PendingAvailability{
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
              }}},
            4,
            {}));
    const auto chain2 = populate_chain_from_previous_storage(
        scope_pending_move_backwards, storage);
    ASSERT_TRUE(chain2.best_chain_vec().empty());
    ASSERT_EQ(chain2.unconnected_len(), 0);
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

  ASSERT_OUTCOME_SUCCESS(
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
  size_t max_depth = 6;
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
  ASSERT_OUTCOME_SUCCESS(
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
    // Back candidate 5 (last one) first - this shouldn't create a chain yet
    chain_new.candidate_backed(candidate_hashes[5]);
    ASSERT_EQ(chain_new.unconnected_len(), 6);
    for (size_t count = 0; count < 10; ++count) {
      ASSERT_EQ(chain_new.find_backable_chain(Ancestors{}, count).size(), 0);
    }

    // Back candidates 3 and 4 - still no chain should form
    chain_new.candidate_backed(candidate_hashes[3]);
    ASSERT_EQ(chain_new.unconnected_len(), 6);
    chain_new.candidate_backed(candidate_hashes[4]);
    ASSERT_EQ(chain_new.unconnected_len(), 6);
    for (size_t count = 0; count < 10; ++count) {
      ASSERT_EQ(chain_new.find_backable_chain(Ancestors{}, count).size(), 0);
    }

    // Back candidate 1 - still no chain
    chain_new.candidate_backed(candidate_hashes[1]);
    ASSERT_EQ(chain_new.unconnected_len(), 6);
    for (size_t count = 0; count < 10; ++count) {
      ASSERT_EQ(chain_new.find_backable_chain(Ancestors{}, count).size(), 0);
    }

    // Back candidate 0 - now we can form a chain with 0 and 1
    chain_new.candidate_backed(candidate_hashes[0]);
    ASSERT_EQ(chain_new.unconnected_len(),
              4);  // 4 unconnected candidates remain
    ASSERT_EQ(chain_new.find_backable_chain(Ancestors{}, 1), hashes(0, 1));
    for (size_t count = 2; count < 10; ++count) {
      ASSERT_EQ(chain_new.find_backable_chain(Ancestors{}, count),
                hashes(0, 2));
    }

    // Now back the missing piece (candidate 2)
    chain_new.candidate_backed(candidate_hashes[2]);
    ASSERT_EQ(chain_new.unconnected_len(),
              0);  // All candidates in the chain now
    ASSERT_EQ(chain_new.best_chain_len(), 6);

    for (size_t count = 0; count < 10; ++count) {
      ASSERT_EQ(chain_new.find_backable_chain(Ancestors{}, count),
                hashes(0, std::min(count, size_t(6))));
    }
  }

  // Now back all candidates in a random order. The result should always be the
  // same.
  auto candidates_shuffled = candidate_hashes;
  std::default_random_engine random_;
  std::shuffle(candidates_shuffled.begin(), candidates_shuffled.end(), random_);
  for (const auto &candidate : candidates_shuffled) {
    chain.candidate_backed(candidate);
    storage.mark_backed(candidate);
  }

  // No ancestors supplied - test different counts
  ASSERT_EQ(chain.find_ancestor_path(Ancestors{}), 0);
  ASSERT_EQ(chain.find_backable_chain(Ancestors{}, 0),
            hashes(0, 0));  // Empty result for count 0
  ASSERT_EQ(chain.find_backable_chain(Ancestors{}, 1),
            hashes(0, 1));  // Just candidate 0
  ASSERT_EQ(chain.find_backable_chain(Ancestors{}, 2),
            hashes(0, 2));  // Candidates 0 and 1
  ASSERT_EQ(chain.find_backable_chain(Ancestors{}, 5),
            hashes(0, 5));  // Candidates 0 through 4

  // For counts larger than the chain length, should return the entire chain
  for (size_t count = 6; count < 10; ++count) {
    ASSERT_EQ(chain.find_backable_chain(Ancestors{}, count), hashes(0, 6));
  }

  // Explicit tests for larger counts
  ASSERT_EQ(chain.find_backable_chain(Ancestors{}, 7), hashes(0, 6));
  ASSERT_EQ(chain.find_backable_chain(Ancestors{}, 10), hashes(0, 6));

  // Ancestor which is not part of the chain. Will be ignored.
  {
    Ancestors ancestors = {CandidateHash{}};
    ASSERT_EQ(chain.find_ancestor_path(ancestors), 0);
    ASSERT_EQ(chain.find_backable_chain(ancestors, 4), hashes(0, 4));
  }

  {
    // Ancestor is candidate 1, but with an invalid candidate - should be
    // ignored
    Ancestors ancestors = {candidate_hashes[1], CandidateHash{}};
    ASSERT_EQ(chain.find_ancestor_path(ancestors), 0);
    ASSERT_EQ(chain.find_backable_chain(ancestors, 4), hashes(0, 4));
  }

  {
    // Ancestor is candidate 0 - should start from position 1
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
    // Valid ancestors in non-sequential order
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
    ASSERT_OUTCOME_SUCCESS(
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
