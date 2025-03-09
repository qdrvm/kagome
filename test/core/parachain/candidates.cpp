/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "parachain/validator/impl/candidates.hpp"
#include "core/parachain/parachain_test_harness.hpp"

using namespace kagome::parachain;

class CandidatesTest : public ProspectiveParachainsTestHarness {
  void SetUp() override {
    ProspectiveParachainsTestHarness::SetUp();
  }

  void TearDown() override {
    ProspectiveParachainsTestHarness::TearDown();
  }

 public:
  template <typename T>
  inline Hash hash_of(const T &t) {
    return hasher_->blake2b_256(encode(std::forward<T>(t)).value());
  }

  inline Hash hash_of(const HeadData &t) {
    return hasher_->blake2b_256(t);
  }

  inline Hash hash_of(const network::CommittedCandidateReceipt &t) {
    return hash(t);
  }

  libp2p::PeerId getPeerFrom(uint64_t i) {
    const auto s = fmt::format("Peer#{}", i);
    libp2p::PeerId peer_id = operator""_peerid(s.data(), s.size());
    return peer_id;
  }

  Hash from_low_u64_be(uint64_t v) {
    Hash h{};
    const uint64_t value = LE_BE_SWAP64(v);
    for (size_t i = 0; i < 8; ++i) {
      h[32 - 8 + i] = ((value >> (i * 8)) & 0xff);
    }
    return h;
  }
};

TEST_F(CandidatesTest, inserting_unconfirmed_rejects_on_incompatible_claims) {
  const HeadData relay_head_data_a = {1, 2, 3};
  const HeadData relay_head_data_b = {4, 5, 6};

  const auto relay_hash_a = hash_of(relay_head_data_a);
  const auto relay_hash_b = hash_of(relay_head_data_b);

  const ParachainId para_id_a = 1;
  const ParachainId para_id_b = 2;

  const auto &[candidate_a, pvd_a] = make_candidate(relay_hash_a,
                                                    1,
                                                    para_id_a,
                                                    relay_head_data_a,
                                                    {1},
                                                    from_low_u64_be(1000));

  const auto candidate_hash_a = hash(candidate_a);

  const auto peer = getPeerFrom(1);

  const GroupIndex group_index_a = 100;
  const GroupIndex group_index_b = 200;

  Candidates candidates;

  // Confirm a candidate first.
  candidates.confirm_candidate(
      candidate_hash_a, candidate_a, pvd_a, group_index_a, hasher_);

  // Relay parent does not match.
  ASSERT_EQ(
      candidates.insert_unconfirmed(peer,
                                    candidate_hash_a,
                                    relay_hash_b,
                                    group_index_a,
                                    std::make_pair(relay_hash_a, para_id_a)),
      false);

  // Group index does not match.
  ASSERT_EQ(
      candidates.insert_unconfirmed(peer,
                                    candidate_hash_a,
                                    relay_hash_a,
                                    group_index_b,
                                    std::make_pair(relay_hash_a, para_id_a)),
      false);

  // Parent head data does not match.
  ASSERT_EQ(
      candidates.insert_unconfirmed(peer,
                                    candidate_hash_a,
                                    relay_hash_a,
                                    group_index_a,
                                    std::make_pair(relay_hash_b, para_id_a)),
      false);

  // Para ID does not match.
  ASSERT_EQ(
      candidates.insert_unconfirmed(peer,
                                    candidate_hash_a,
                                    relay_hash_a,
                                    group_index_a,
                                    std::make_pair(relay_hash_a, para_id_b)),
      false);

  // Everything matches.
  ASSERT_EQ(
      candidates.insert_unconfirmed(peer,
                                    candidate_hash_a,
                                    relay_hash_a,
                                    group_index_a,
                                    std::make_pair(relay_hash_a, para_id_a)),
      true);
}

TEST_F(CandidatesTest, confirming_maintains_parent_hash_index) {
  const HeadData relay_head_data = {1, 2, 3};
  const auto relay_hash = hash_of(relay_head_data);

  const HeadData candidate_head_data_a = {1};
  const HeadData candidate_head_data_b = {2};
  const HeadData candidate_head_data_c = {3};
  const HeadData candidate_head_data_d = {4};

  const auto candidate_head_data_hash_a = hash_of(candidate_head_data_a);
  const auto candidate_head_data_hash_b = hash_of(candidate_head_data_b);
  const auto candidate_head_data_hash_c = hash_of(candidate_head_data_c);

  const auto &[candidate_a, pvd_a] = make_candidate(relay_hash,
                                                    1,
                                                    1,
                                                    relay_head_data,
                                                    candidate_head_data_a,
                                                    from_low_u64_be(1000));
  const auto &[candidate_b, pvd_b] = make_candidate(relay_hash,
                                                    1,
                                                    1,
                                                    candidate_head_data_a,
                                                    candidate_head_data_b,
                                                    from_low_u64_be(2000));
  const auto &[candidate_c, _] = make_candidate(relay_hash,
                                                1,
                                                1,
                                                candidate_head_data_b,
                                                candidate_head_data_c,
                                                from_low_u64_be(3000));

  const auto &[candidate_d, pvd_d] = make_candidate(relay_hash,
                                                    1,
                                                    1,
                                                    candidate_head_data_c,
                                                    candidate_head_data_d,
                                                    from_low_u64_be(4000));

  const auto candidate_hash_a = hash_of(candidate_a);
  const auto candidate_hash_b = hash_of(candidate_b);
  const auto candidate_hash_c = hash_of(candidate_c);
  const auto candidate_hash_d = hash_of(candidate_d);

  const auto peer = getPeerFrom(1);
  const GroupIndex group_index = 100;

  Candidates candidates;

  // Insert some unconfirmed candidates.

  // Advertise A without parent hash.
  ASSERT_TRUE(candidates.insert_unconfirmed(
      peer, candidate_hash_a, relay_hash, group_index, std::nullopt));

  ASSERT_EQ(candidates.by_parent, Candidates::ByRelayParent{});

  // Advertise A with parent hash and ID.
  ASSERT_TRUE(candidates.insert_unconfirmed(peer,
                                            candidate_hash_a,
                                            relay_hash,
                                            group_index,
                                            std::make_pair(relay_hash, 1)));

  auto ASSERT_RESULT = [&](const Candidates::ByRelayParent &ref) -> auto {
    ASSERT_EQ(candidates.by_parent, ref);
  };

  ASSERT_RESULT({{relay_hash, {{1, {candidate_hash_a}}}}});

  // Advertise B with parent A.
  ASSERT_TRUE(candidates.insert_unconfirmed(
      peer,
      candidate_hash_b,
      relay_hash,
      group_index,
      std::make_pair(candidate_head_data_hash_a, 1)));
  ASSERT_RESULT({{relay_hash, {{1, {candidate_hash_a}}}},
                 {candidate_head_data_hash_a, {{1, {candidate_hash_b}}}}});

  // Advertise C with parent A.
  ASSERT_TRUE(candidates.insert_unconfirmed(
      peer,
      candidate_hash_c,
      relay_hash,
      group_index,
      std::make_pair(candidate_head_data_hash_a, 1)));
  ASSERT_RESULT({{relay_hash, {{1, {candidate_hash_a}}}},
                 {candidate_head_data_hash_a,
                  {{1, {candidate_hash_b, candidate_hash_c}}}}});

  // Advertise D with parent A.
  ASSERT_TRUE(candidates.insert_unconfirmed(
      peer,
      candidate_hash_d,
      relay_hash,
      group_index,
      std::make_pair(candidate_head_data_hash_a, 1)));
  ASSERT_RESULT(
      {{relay_hash, {{1, {candidate_hash_a}}}},
       {candidate_head_data_hash_a,
        {{1, {candidate_hash_b, candidate_hash_c, candidate_hash_d}}}}});

  // Insert confirmed candidates and check parent hash index.

  // Confirmation matches advertisement. Index should be unchanged.
  candidates.confirm_candidate(
      candidate_hash_a, candidate_a, pvd_a, group_index, hasher_);
  ASSERT_RESULT(
      {{relay_hash, {{1, {candidate_hash_a}}}},
       {candidate_head_data_hash_a,
        {{1, {candidate_hash_b, candidate_hash_c, candidate_hash_d}}}}});

  candidates.confirm_candidate(
      candidate_hash_b, candidate_b, pvd_b, group_index, hasher_);
  ASSERT_RESULT(
      {{relay_hash, {{1, {candidate_hash_a}}}},
       {candidate_head_data_hash_a,
        {{1, {candidate_hash_b, candidate_hash_c, candidate_hash_d}}}}});

  // Confirmation does not match advertisement. Index should be updated.
  candidates.confirm_candidate(
      candidate_hash_d, candidate_d, pvd_d, group_index, hasher_);
  ASSERT_RESULT({{relay_hash, {{1, {candidate_hash_a}}}},
                 {candidate_head_data_hash_a,
                  {{1, {candidate_hash_b, candidate_hash_c}}}},
                 {candidate_head_data_hash_c, {{1, {candidate_hash_d}}}}});

  // Make a new candidate for C with a different para ID.
  const auto &[new_candidate_c, new_pvd_c] =
      make_candidate(relay_hash,
                     1,
                     2,
                     candidate_head_data_b,
                     candidate_head_data_c,
                     from_low_u64_be(3000));
  candidates.confirm_candidate(
      candidate_hash_c, new_candidate_c, new_pvd_c, group_index, hasher_);
  ASSERT_RESULT({{relay_hash, {{1, {candidate_hash_a}}}},
                 {candidate_head_data_hash_a, {{1, {candidate_hash_b}}}},
                 {candidate_head_data_hash_b, {{2, {candidate_hash_c}}}},
                 {candidate_head_data_hash_c, {{1, {candidate_hash_d}}}}});
}

TEST_F(CandidatesTest, test_returned_post_confirmation) {
  const HeadData relay_head_data = {1, 2, 3};
  const auto relay_hash = hash_of(relay_head_data);

  const HeadData candidate_head_data_a = {1};
  const HeadData candidate_head_data_b = {2};
  const HeadData candidate_head_data_c = {3};
  const HeadData candidate_head_data_d = {4};

  const auto candidate_head_data_hash_a = hash_of(candidate_head_data_a);
  const auto candidate_head_data_hash_b = hash_of(candidate_head_data_b);

  const auto &[candidate_a, pvd_a] = make_candidate(relay_hash,
                                                    1,
                                                    1,
                                                    relay_head_data,
                                                    candidate_head_data_a,
                                                    from_low_u64_be(1000));
  const auto &[candidate_b, pvd_b] = make_candidate(relay_hash,
                                                    1,
                                                    1,
                                                    candidate_head_data_a,
                                                    candidate_head_data_b,
                                                    from_low_u64_be(2000));
  const auto &[candidate_c, _] = make_candidate(relay_hash,
                                                1,
                                                1,
                                                candidate_head_data_a,
                                                candidate_head_data_c,
                                                from_low_u64_be(3000));
  const auto &[candidate_d, pvd_d] = make_candidate(relay_hash,
                                                    1,
                                                    1,
                                                    candidate_head_data_b,
                                                    candidate_head_data_d,
                                                    from_low_u64_be(4000));

  const auto candidate_hash_a = hash_of(candidate_a);
  const auto candidate_hash_b = hash_of(candidate_b);
  const auto candidate_hash_c = hash_of(candidate_c);
  const auto candidate_hash_d = hash_of(candidate_d);

  const auto peer_a = getPeerFrom(1);
  const auto peer_b = getPeerFrom(2);
  const auto peer_c = getPeerFrom(3);
  const auto peer_d = getPeerFrom(4);

  const GroupIndex group_index = 100;

  Candidates candidates;

  // Insert some unconfirmed candidates.

  // Advertise A without parent hash.
  ASSERT_TRUE(candidates.insert_unconfirmed(
      peer_a, candidate_hash_a, relay_hash, group_index, std::nullopt));

  // Advertise A with parent hash and ID.
  ASSERT_TRUE(candidates.insert_unconfirmed(peer_a,
                                            candidate_hash_a,
                                            relay_hash,
                                            group_index,
                                            std::make_pair(relay_hash, 1)));

  // (Correctly) advertise B with parent A. Do it from a couple of peers.
  ASSERT_TRUE(candidates.insert_unconfirmed(
      peer_a,
      candidate_hash_b,
      relay_hash,
      group_index,
      std::make_pair(candidate_head_data_hash_a, 1)));
  ASSERT_TRUE(candidates.insert_unconfirmed(
      peer_b,
      candidate_hash_b,
      relay_hash,
      group_index,
      std::make_pair(candidate_head_data_hash_a, 1)));

  // (Wrongly) advertise C with parent A. Do it from a couple peers.
  ASSERT_TRUE(candidates.insert_unconfirmed(
      peer_b,
      candidate_hash_c,
      relay_hash,
      group_index,
      std::make_pair(candidate_head_data_hash_a, 1)));
  ASSERT_TRUE(candidates.insert_unconfirmed(
      peer_c,
      candidate_hash_c,
      relay_hash,
      group_index,
      std::make_pair(candidate_head_data_hash_a, 1)));

  // Advertise D. Do it correctly from one peer (parent B) and wrongly from
  // another (parent A).
  ASSERT_TRUE(candidates.insert_unconfirmed(
      peer_c,
      candidate_hash_d,
      relay_hash,
      group_index,
      std::make_pair(candidate_head_data_hash_b, 1)));
  ASSERT_TRUE(candidates.insert_unconfirmed(
      peer_d,
      candidate_hash_d,
      relay_hash,
      group_index,
      std::make_pair(candidate_head_data_hash_a, 1)));

  auto ASSERT_RESULT = [&](const Candidates::ByRelayParent &ref) -> auto {
    ASSERT_EQ(candidates.by_parent, ref);
  };

  ASSERT_RESULT(
      {{relay_hash, {{1, {candidate_hash_a}}}},
       {candidate_head_data_hash_a,
        {{1, {candidate_hash_b, candidate_hash_c, candidate_hash_d}}}},
       {candidate_head_data_hash_b, {{1, {candidate_hash_d}}}}});

  // Insert confirmed candidates and check parent hash index.

  // Confirmation matches advertisement.
  {
    const auto post_confirmation = candidates.confirm_candidate(
        candidate_hash_a, candidate_a, pvd_a, group_index, hasher_);
    ASSERT_EQ(post_confirmation,
              std::make_optional(PostConfirmation{
                  .hypothetical =
                      HypotheticalCandidateComplete{
                          .candidate_hash = candidate_hash_a,
                          .receipt = candidate_a,
                          .persisted_validation_data = pvd_a,
                      },
                  .reckoning =
                      PostConfirmationReckoning{
                          .correct = {peer_a},
                          .incorrect = {},
                      },
              }));
  }

  {
    const auto post_confirmation = candidates.confirm_candidate(
        candidate_hash_b, candidate_b, pvd_b, group_index, hasher_);
    ASSERT_EQ(post_confirmation,
              std::make_optional(PostConfirmation{
                  .hypothetical =
                      HypotheticalCandidateComplete{
                          .candidate_hash = candidate_hash_b,
                          .receipt = candidate_b,
                          .persisted_validation_data = pvd_b,
                      },
                  .reckoning =
                      PostConfirmationReckoning{
                          .correct = {peer_a, peer_b},
                          .incorrect = {},
                      },
              }));
  }

  // Confirm candidate with two wrong peers (different group index).
  const auto &[new_candidate_c, new_pvd_c] =
      make_candidate(relay_hash,
                     1,
                     2,
                     candidate_head_data_b,
                     candidate_head_data_c,
                     from_low_u64_be(3000));

  {
    const auto post_confirmation = candidates.confirm_candidate(
        candidate_hash_c, new_candidate_c, new_pvd_c, group_index, hasher_);
    ASSERT_EQ(post_confirmation,
              std::make_optional(PostConfirmation{
                  .hypothetical =
                      HypotheticalCandidateComplete{
                          .candidate_hash = candidate_hash_c,
                          .receipt = new_candidate_c,
                          .persisted_validation_data = new_pvd_c,
                      },
                  .reckoning =
                      PostConfirmationReckoning{
                          .correct = {},
                          .incorrect = {peer_b, peer_c},
                      },
              }));
  }

  // Confirm candidate with one wrong peer (different parent head data).
  {
    const auto post_confirmation = candidates.confirm_candidate(
        candidate_hash_d, candidate_d, pvd_d, group_index, hasher_);
    ASSERT_EQ(post_confirmation,
              std::make_optional(PostConfirmation{
                  .hypothetical =
                      HypotheticalCandidateComplete{
                          .candidate_hash = candidate_hash_d,
                          .receipt = candidate_d,
                          .persisted_validation_data = pvd_d,
                      },
                  .reckoning =
                      PostConfirmationReckoning{
                          .correct = {peer_c},
                          .incorrect = {peer_d},
                      },
              }));
  }
}

TEST_F(CandidatesTest, test_hypothetical_frontiers) {
  const HeadData relay_head_data = {1, 2, 3};
  const auto relay_hash = hash_of(relay_head_data);

  const HeadData candidate_head_data_a = {1};
  const HeadData candidate_head_data_b = {2};
  const HeadData candidate_head_data_c = {3};
  const HeadData candidate_head_data_d = {4};

  const auto candidate_head_data_hash_a = hash_of(candidate_head_data_a);
  const auto candidate_head_data_hash_b = hash_of(candidate_head_data_b);
  const auto candidate_head_data_hash_d = hash_of(candidate_head_data_d);

  const auto &[candidate_a, pvd_a] = make_candidate(relay_hash,
                                                    1,
                                                    1,
                                                    relay_head_data,
                                                    candidate_head_data_a,
                                                    from_low_u64_be(1000));
  const auto &[candidate_b, _] = make_candidate(relay_hash,
                                                1,
                                                1,
                                                candidate_head_data_a,
                                                candidate_head_data_b,
                                                from_low_u64_be(2000));
  const auto &[candidate_c, __] = make_candidate(relay_hash,
                                                 1,
                                                 1,
                                                 candidate_head_data_a,
                                                 candidate_head_data_c,
                                                 from_low_u64_be(3000));
  const auto &[candidate_d, ___] = make_candidate(relay_hash,
                                                  1,
                                                  1,
                                                  candidate_head_data_b,
                                                  candidate_head_data_d,
                                                  from_low_u64_be(4000));

  const auto candidate_hash_a = hash_of(candidate_a);
  const auto candidate_hash_b = hash_of(candidate_b);
  const auto candidate_hash_c = hash_of(candidate_c);
  const auto candidate_hash_d = hash_of(candidate_d);

  const auto peer = getPeerFrom(1);
  const GroupIndex group_index = 100;

  Candidates candidates;

  // Confirm A.
  candidates.confirm_candidate(
      candidate_hash_a, candidate_a, pvd_a, group_index, hasher_);

  // Advertise B with parent A.
  ASSERT_TRUE(candidates.insert_unconfirmed(
      peer,
      candidate_hash_b,
      relay_hash,
      group_index,
      std::make_pair(candidate_head_data_hash_a, 1)));

  // Advertise C with parent A.
  ASSERT_TRUE(candidates.insert_unconfirmed(
      peer,
      candidate_hash_c,
      relay_hash,
      group_index,
      std::make_pair(candidate_head_data_hash_a, 1)));

  // Advertise D with parent B.
  ASSERT_TRUE(candidates.insert_unconfirmed(
      peer,
      candidate_hash_d,
      relay_hash,
      group_index,
      std::make_pair(candidate_head_data_hash_b, 1)));

  auto ASSERT_RESULT = [&](const Candidates::ByRelayParent &ref) -> auto {
    ASSERT_EQ(candidates.by_parent, ref);
  };

  ASSERT_RESULT({{relay_hash, {{1, {candidate_hash_a}}}},
                 {candidate_head_data_hash_a,
                  {{1, {candidate_hash_b, candidate_hash_c}}}},
                 {candidate_head_data_hash_b, {{1, {candidate_hash_d}}}}});

  const HypotheticalCandidateComplete hypothetical_a{
      .candidate_hash = candidate_hash_a,
      .receipt = candidate_a,
      .persisted_validation_data = pvd_a,
  };
  const HypotheticalCandidateIncomplete hypothetical_b{
      .candidate_hash = candidate_hash_b,
      .candidate_para = 1,
      .parent_head_data_hash = candidate_head_data_hash_a,
      .candidate_relay_parent = relay_hash,
  };
  const HypotheticalCandidateIncomplete hypothetical_c{
      .candidate_hash = candidate_hash_c,
      .candidate_para = 1,
      .parent_head_data_hash = candidate_head_data_hash_a,
      .candidate_relay_parent = relay_hash,
  };
  const HypotheticalCandidateIncomplete hypothetical_d{
      .candidate_hash = candidate_hash_d,
      .candidate_para = 1,
      .parent_head_data_hash = candidate_head_data_hash_b,
      .candidate_relay_parent = relay_hash,
  };

  auto CONTAINS = [](const std::vector<HypotheticalCandidate> &a,
                     const HypotheticalCandidate &v) -> bool {
    for (const auto &ai : a) {
      if (ai == v) {
        return true;
      }
    }
    return false;
  };

  {
    const auto hypotheticals =
        candidates.frontier_hypotheticals(std::make_pair(relay_hash, 1));
    ASSERT_EQ(hypotheticals.size(), 1);
    ASSERT_TRUE(hypotheticals[0] == HypotheticalCandidate(hypothetical_a));
  }

  {
    const auto hypotheticals = candidates.frontier_hypotheticals(
        std::make_pair(candidate_head_data_hash_a, 2));
    ASSERT_EQ(hypotheticals.size(), 0);
  }

  {
    const auto hypotheticals = candidates.frontier_hypotheticals(
        std::make_pair(candidate_head_data_hash_a, 1));
    ASSERT_EQ(hypotheticals.size(), 2);
    ASSERT_TRUE(CONTAINS(hypotheticals, hypothetical_b));
    ASSERT_TRUE(CONTAINS(hypotheticals, hypothetical_c));
  }

  {
    const auto hypotheticals = candidates.frontier_hypotheticals(
        std::make_pair(candidate_head_data_hash_d, 1));
    ASSERT_EQ(hypotheticals.size(), 0);
  }

  {
    const auto hypotheticals = candidates.frontier_hypotheticals(std::nullopt);
    ASSERT_EQ(hypotheticals.size(), 4);
    ASSERT_TRUE(CONTAINS(hypotheticals, hypothetical_a));
    ASSERT_TRUE(CONTAINS(hypotheticals, hypothetical_b));
    ASSERT_TRUE(CONTAINS(hypotheticals, hypothetical_c));
    ASSERT_TRUE(CONTAINS(hypotheticals, hypothetical_d));
  }
}
