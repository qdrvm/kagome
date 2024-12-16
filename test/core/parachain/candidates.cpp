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
    return hasher_->blake2b_256(scale::encode(std::forward<T>(t)).value());
  }

  template <>
  inline Hash hash_of<HeadData>(const HeadData &t) {
    return hasher_->blake2b_256(t);
  }

  template <>
  inline Hash hash_of<network::CommittedCandidateReceipt>(const network::CommittedCandidateReceipt &t) {
    return hash(t);
  }

    libp2p::PeerId getPeerFrom(uint64_t i) {
        const auto s = fmt::format("Peer#{}", i);
        libp2p::PeerId peer_id = operator""_peerid(s.data(), s.size());
        return peer_id;
    }

    template <typename T>
    void println(std::string_view pref, const T &t) {
        std::cout << fmt::format("{}   ===>>>   {}\n", pref, t);
    }

    Hash from_low_u64_be(uint64_t v) {
        Hash h{};
        *(uint64_t*)&h[32 - 8] = LE_BE_SWAP64(v);
        return h;
    }
};

TEST_F(CandidatesTest, inserting_unconfirmed_rejects_on_incompatible_claims) {
		const HeadData relay_head_data_a = {1, 2, 3};
		const HeadData relay_head_data_b = {4, 5, 6};

		const auto relay_hash_a = hash_of(relay_head_data_a);
		const auto relay_hash_b = hash_of(relay_head_data_b);

        println("relay_hash_a", relay_hash_a);
        println("relay_hash_b", relay_hash_b);

		const ParachainId para_id_a = 1;
		const ParachainId para_id_b = 2;

		const auto &[candidate_a, pvd_a] = make_candidate(
			relay_hash_a,
			1,
			para_id_a,
			relay_head_data_a,
			{1},
			from_low_u64_be(1000)
		);

		const auto candidate_hash_a = hash(candidate_a);
        println("candidate_hash_a", candidate_hash_a);

		const auto peer = getPeerFrom(1);

		const GroupIndex group_index_a = 100;
		const GroupIndex group_index_b = 200;

		Candidates candidates;

		 // Confirm a candidate first.
		 candidates.confirm_candidate(candidate_hash_a, candidate_a, pvd_a, group_index_a, hasher_);

		// Relay parent does not match.
		ASSERT_EQ(
			candidates.insert_unconfirmed(
				peer,
				candidate_hash_a,
				relay_hash_b,
				group_index_a,
				std::make_pair(relay_hash_a, para_id_a)
			),
			false
		);

		// Group index does not match.
		ASSERT_EQ(
			candidates.insert_unconfirmed(
				peer,
				candidate_hash_a,
				relay_hash_a,
				group_index_b,
				std::make_pair(relay_hash_a, para_id_a)
			),
			false
		);

		// Parent head data does not match.
		ASSERT_EQ(
			candidates.insert_unconfirmed(
				peer,
				candidate_hash_a,
				relay_hash_a,
				group_index_a,
				std::make_pair(relay_hash_b, para_id_a)
			),
			false
		);

		// Para ID does not match.
		ASSERT_EQ(
			candidates.insert_unconfirmed(
				peer,
				candidate_hash_a,
				relay_hash_a,
				group_index_a,
				std::make_pair(relay_hash_a, para_id_b)
			),
			false
		);

		// Everything matches.
		ASSERT_EQ(
			candidates.insert_unconfirmed(
				peer,
				candidate_hash_a,
				relay_hash_a,
				group_index_a,
				std::make_pair(relay_hash_a, para_id_a)
			),
			true
		);
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

		const auto &[candidate_a, pvd_a] = make_candidate(
			relay_hash,
			1,
			1,
			relay_head_data,
			candidate_head_data_a,
			from_low_u64_be(1000)
		);
		const auto &[candidate_b, pvd_b] = make_candidate(
			relay_hash,
			1,
			1,
			candidate_head_data_a,
			candidate_head_data_b,
			from_low_u64_be(2000)
		);
		const auto &[candidate_c, _] = make_candidate(
			relay_hash,
			1,
			1,
			candidate_head_data_b,
			candidate_head_data_c,
			from_low_u64_be(3000)
		);

		const auto &[candidate_d, pvd_d] = make_candidate(
			relay_hash,
			1,
			1,
			candidate_head_data_c,
			candidate_head_data_d,
			from_low_u64_be(4000)
		);

		const auto candidate_hash_a = hash_of(candidate_a);
		const auto candidate_hash_b = hash_of(candidate_b);
		const auto candidate_hash_c = hash_of(candidate_c);
		const auto candidate_hash_d = hash_of(candidate_d);

        const auto peer = getPeerFrom(1);
		const GroupIndex group_index = 100;

		Candidates candidates;

		// Insert some unconfirmed candidates.

		// Advertise A without parent hash.
		ASSERT_TRUE(candidates
			.insert_unconfirmed(peer, candidate_hash_a, relay_hash, group_index, std::nullopt));

		ASSERT_EQ(candidates.by_parent, Candidates::ByRelayParent{});

		// Advertise A with parent hash and ID.
		ASSERT_TRUE(candidates
			.insert_unconfirmed(
				peer,
				candidate_hash_a,
				relay_hash,
				group_index,
				std::make_pair(relay_hash, 1)
			));

        auto ASSERT_RESULT = [&] (const Candidates::ByRelayParent &ref) -> auto {
		    ASSERT_EQ(candidates.by_parent, ref);
        };

        ASSERT_RESULT({{relay_hash, {{1, {candidate_hash_a}}}}});

		// Advertise B with parent A.
		ASSERT_TRUE(candidates
			.insert_unconfirmed(
				peer,
				candidate_hash_b,
				relay_hash,
				group_index,
				std::make_pair(candidate_head_data_hash_a, 1)
			));
        ASSERT_RESULT({
            {relay_hash, {{1, {candidate_hash_a}}}},
            {candidate_head_data_hash_a, {{1, {candidate_hash_b}}}}
        });

		// Advertise C with parent A.
		ASSERT_TRUE(candidates
			.insert_unconfirmed(
				peer,
				candidate_hash_c,
				relay_hash,
				group_index,
				std::make_pair(candidate_head_data_hash_a, 1)
			));
        ASSERT_RESULT({
            {relay_hash, {{1, {candidate_hash_a}}}},
            {candidate_head_data_hash_a, {{1, {candidate_hash_b, candidate_hash_c}}}}
        });

		// Advertise D with parent A.
		ASSERT_TRUE(candidates
			.insert_unconfirmed(
				peer,
				candidate_hash_d,
				relay_hash,
				group_index,
				std::make_pair(candidate_head_data_hash_a, 1)
			));
        ASSERT_RESULT({
            {relay_hash, {{1, {candidate_hash_a}}}},
            {candidate_head_data_hash_a, {{1, {candidate_hash_b, candidate_hash_c, candidate_hash_d}}}}
        });


		// Insert confirmed candidates and check parent hash index.

		// Confirmation matches advertisement. Index should be unchanged.
		candidates.confirm_candidate(candidate_hash_a, candidate_a, pvd_a, group_index, hasher_);
        ASSERT_RESULT({
            {relay_hash, {{1, {candidate_hash_a}}}},
            {candidate_head_data_hash_a, {{1, {candidate_hash_b, candidate_hash_c, candidate_hash_d}}}}
        });

		candidates.confirm_candidate(candidate_hash_b, candidate_b, pvd_b, group_index, hasher_);
        ASSERT_RESULT({
            {relay_hash, {{1, {candidate_hash_a}}}},
            {candidate_head_data_hash_a, {{1, {candidate_hash_b, candidate_hash_c, candidate_hash_d}}}}
        });

		// Confirmation does not match advertisement. Index should be updated.
		candidates.confirm_candidate(candidate_hash_d, candidate_d, pvd_d, group_index, hasher_);
        ASSERT_RESULT({
            {relay_hash, {{1, {candidate_hash_a}}}},
            {candidate_head_data_hash_a, {{1, {candidate_hash_b, candidate_hash_c}}}},
            {candidate_head_data_hash_c, {{1, {candidate_hash_d}}}}
        });

		// Make a new candidate for C with a different para ID.
		const auto &[new_candidate_c, new_pvd_c] = make_candidate(
			relay_hash,
			1,
			2,
			candidate_head_data_b,
			candidate_head_data_c,
			from_low_u64_be(3000)
		);
		candidates.confirm_candidate(candidate_hash_c, new_candidate_c, new_pvd_c, group_index, hasher_);
        ASSERT_RESULT({
            {relay_hash, {{1, {candidate_hash_a}}}},
            {candidate_head_data_hash_a, {{1, {candidate_hash_b}}}},
            {candidate_head_data_hash_b, {{2, {candidate_hash_c}}}},
            {candidate_head_data_hash_c, {{1, {candidate_hash_d}}}}
        });
}