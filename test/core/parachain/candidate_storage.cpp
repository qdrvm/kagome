/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "core/parachain/parachain_test_harness.hpp"
#include "parachain/validator/prospective_parachains/fragment_chain.hpp"

using namespace kagome::parachain::fragment;

class CandidateStorageTest : public ProspectiveParachainsTestHarness {
  void SetUp() override {
    ProspectiveParachainsTestHarness::SetUp();
  }

  void TearDown() override {
    ProspectiveParachainsTestHarness::TearDown();
  }
};

TEST_F(CandidateStorageTest, candidate_storage_methods) {
  fragment::CandidateStorage storage{};
  Hash relay_parent(hashFromStrData("69"));

  const auto &[pvd, candidate] =
      make_committed_candidate(5, relay_parent, 8, {4, 5, 6}, {1, 2, 3}, 7);

  const Hash candidate_hash = network::candidateHash(*hasher_, candidate);
  const Hash parent_head_hash = hasher_->blake2b_256(pvd.get().parent_head);

  // Invalid pvd hash
  auto wrong_pvd = pvd;
  wrong_pvd.get_mut().max_pov_size = 0;

  ASSERT_EQ(
      fragment::CandidateEntry::create(candidate_hash,
                                       candidate,
                                       wrong_pvd.get(),
                                       fragment::CandidateState::Seconded,
                                       hasher_)
          .error(),
      fragment::CandidateStorage::Error::PERSISTED_VALIDATION_DATA_MISMATCH);
  ASSERT_EQ(
      fragment::CandidateEntry::create_seconded(
          candidate_hash, candidate, wrong_pvd.get(), hasher_)
          .error(),
      fragment::CandidateStorage::Error::PERSISTED_VALIDATION_DATA_MISMATCH);

  // Zero-length cycle.
  {
    auto candidate_2 = candidate;
    candidate_2.commitments.para_head = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1};

    auto pvd_2 = pvd;
    pvd_2.get_mut().parent_head = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1};

    candidate_2.descriptor.persisted_data_hash = pvd_2.getHash();
    ASSERT_EQ(fragment::CandidateEntry::create_seconded(
                  candidate_hash, candidate_2, pvd_2.get(), hasher_)
                  .error(),
              fragment::CandidateStorage::Error::ZERO_LENGTH_CYCLE);
  }

  auto calculate_count = [&](const Hash &h) {
    size_t count = 0;
    storage.possible_backed_para_children(h, [&](const auto &) { ++count; });
    return count;
  };

  auto aggregate_hashes = [&](const Hash &h) {
    std::unordered_set<Hash> data;
    storage.possible_backed_para_children(
        h, [&](const auto &c) { data.emplace(c.candidate_hash); });
    return data;
  };

  ASSERT_FALSE(storage.contains(candidate_hash));
  ASSERT_EQ(calculate_count(parent_head_hash), 0);
  ASSERT_EQ(storage.head_data_by_hash(candidate.descriptor.para_head_hash),
            std::nullopt);
  ASSERT_EQ(storage.head_data_by_hash(parent_head_hash), std::nullopt);

  // Add a valid candidate.
  ASSERT_OUTCOME_SUCCESS(
      candidate_entry,
      fragment::CandidateEntry::create(candidate_hash,
                                       candidate,
                                       pvd.get(),
                                       fragment::CandidateState::Seconded,
                                       hasher_));
  std::ignore = storage.add_candidate_entry(candidate_entry);
  ASSERT_TRUE(storage.contains(candidate_hash));

  ASSERT_EQ(calculate_count(parent_head_hash), 0);
  ASSERT_EQ(calculate_count(candidate.descriptor.para_head_hash), 0);
  ASSERT_EQ(storage.head_data_by_hash(candidate.descriptor.para_head_hash)
                .value()
                .get(),
            candidate.commitments.para_head);
  ASSERT_EQ(storage.head_data_by_hash(parent_head_hash).value().get(),
            pvd.get().parent_head);

  // Now mark it as backed
  storage.mark_backed(candidate_hash);
  // Marking it twice is fine.
  storage.mark_backed(candidate_hash);
  ASSERT_EQ(aggregate_hashes(parent_head_hash),
            std::unordered_set<Hash>{candidate_hash});
  ASSERT_EQ(calculate_count(candidate.descriptor.para_head_hash), 0);

  // Re-adding a candidate fails.
  ASSERT_EQ(storage.add_candidate_entry(candidate_entry).error(),
            fragment::CandidateStorage::Error::CANDIDATE_ALREADY_KNOWN);

  // Remove candidate and re-add it later in backed state.
  storage.remove_candidate(candidate_hash, hasher_);
  ASSERT_FALSE(storage.contains(candidate_hash));

  // Removing it twice is fine.
  storage.remove_candidate(candidate_hash, hasher_);
  ASSERT_FALSE(storage.contains(candidate_hash));
  ASSERT_EQ(calculate_count(parent_head_hash), 0);
  ASSERT_EQ(storage.head_data_by_hash(candidate.descriptor.para_head_hash),
            std::nullopt);
  ASSERT_EQ(storage.head_data_by_hash(parent_head_hash), std::nullopt);

  std::ignore = storage.add_pending_availability_candidate(
      candidate_hash, candidate, pvd.get(), hasher_);
  ASSERT_TRUE(storage.contains(candidate_hash));

  ASSERT_EQ(aggregate_hashes(parent_head_hash),
            std::unordered_set<Hash>{candidate_hash});
  ASSERT_EQ(calculate_count(candidate.descriptor.para_head_hash), 0);

  // Now add a second candidate in Seconded state. This will be a fork.
  const auto &[pvd_2, candidate_2] =
      make_committed_candidate(5, relay_parent, 8, {4, 5, 6}, {2, 3, 4}, 7);

  const Hash candidate_hash_2 = network::candidateHash(*hasher_, candidate_2);
  ASSERT_OUTCOME_SUCCESS(
      candidate_entry_2,
      fragment::CandidateEntry::create_seconded(
          candidate_hash_2, candidate_2, pvd_2.get(), hasher_));

  std::ignore = storage.add_candidate_entry(candidate_entry_2);
  ASSERT_EQ(aggregate_hashes(parent_head_hash),
            std::unordered_set<Hash>{candidate_hash});

  // Now mark it as backed.
  storage.mark_backed(candidate_hash_2);
  ASSERT_EQ(aggregate_hashes(parent_head_hash),
            (std::unordered_set<Hash>{candidate_hash, candidate_hash_2}));
}
