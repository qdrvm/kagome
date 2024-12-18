/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <map>
#include <ranges>
#include <vector>

#include "core/parachain/parachain_test_harness.hpp"
#include "parachain/validator/parachain_processor.hpp"
#include "parachain/validator/prospective_parachains/prospective_parachains.hpp"

using namespace kagome::parachain;
namespace runtime = kagome::runtime;

using kagome::runtime::ClaimQueueSnapshot;

class ProspectiveParachainsTest : public ProspectiveParachainsTestHarness {
  void SetUp() override {
    ProspectiveParachainsTestHarness::SetUp();
    parachain_api_ = std::make_shared<runtime::ParachainHostMock>();
    prospective_parachain_ = std::make_shared<ProspectiveParachains>(
        hasher_, parachain_api_, block_tree_);
  }

  void TearDown() override {
    prospective_parachain_.reset();
    parachain_api_.reset();
    ProspectiveParachainsTestHarness::TearDown();
  }

 public:
  using ClaimQueue = std::map<CoreIndex, std::vector<ParachainId>>;

  static constexpr fragment::AsyncBackingParams ASYNC_BACKING_PARAMETERS{
      .max_candidate_depth = 4,
      .allowed_ancestry_len = ALLOWED_ANCESTRY_LEN,
  };

  std::shared_ptr<runtime::ParachainHostMock> parachain_api_;
  std::shared_ptr<ProspectiveParachains> prospective_parachain_;

  struct TestState {
    ClaimQueue claim_queue = {{CoreIndex(0), {ParachainId(1)}},
                              {CoreIndex(1), {ParachainId(2)}}};
    bool enable_claim_queue_api = true;
    ValidationCodeHash validation_code_hash = fromNumber(42);
  };

  struct PerParaData {
    BlockNumber min_relay_parent;
    HeadData head_data;
    std::vector<fragment::CandidatePendingAvailability> pending_availability;

    PerParaData(BlockNumber min_relay_parent_, const HeadData &head_data_)
        : min_relay_parent{min_relay_parent_}, head_data{head_data_} {}

    PerParaData(
        BlockNumber min_relay_parent_,
        const HeadData &head_data_,
        const std::vector<fragment::CandidatePendingAvailability> &pending_)
        : min_relay_parent{min_relay_parent_},
          head_data{head_data_},
          pending_availability{pending_} {}
  };

  struct TestLeaf {
    BlockNumber number;
    Hash hash;
    std::vector<std::pair<ParachainId, PerParaData>> para_data;

    std::reference_wrapper<const PerParaData> paraData(
        ParachainId para_id) const {
      for (const auto &[para, per_data] : para_data) {
        if (para == para_id) {
          return {per_data};
        }
      }
      UNREACHABLE;
    }
  };

  fragment::Constraints dummy_constraints(
      BlockNumber min_relay_parent_number,
      std::vector<BlockNumber> valid_watermarks,
      const HeadData &required_parent,
      const ValidationCodeHash &validation_code_hash) {
    return fragment::Constraints{
        .min_relay_parent_number = min_relay_parent_number,
        .max_pov_size = MAX_POV_SIZE,
        .max_code_size = 1000000,
        .ump_remaining = 10,
        .ump_remaining_bytes = 1000,
        .max_ump_num_per_candidate = 10,
        .dmp_remaining_messages = {},
        .hrmp_inbound =
            fragment::InboundHrmpLimitations{
                .valid_watermarks = valid_watermarks,
            },
        .hrmp_channels_out = {},
        .max_hrmp_num_per_candidate = 0,
        .required_parent = required_parent,
        .validation_code_hash = validation_code_hash,
        .upgrade_restriction = {},
        .future_validation_code = {},
    };
  }

  void handle_leaf_activation_2(
      const network::ExView &update,
      const TestLeaf &leaf,
      const TestState &test_state,
      const fragment::AsyncBackingParams &async_backing_params,
      std::function<Hash(const Hash &)> func = get_parent_hash) {
    const auto &[number, hash, para_data] = leaf;
    const auto &header = update.new_head;

    EXPECT_CALL(*parachain_api_, staging_async_backing_params(hash))
        .WillRepeatedly(Return(outcome::success(async_backing_params)));

    if (not test_state.enable_claim_queue_api) {
      EXPECT_CALL(*parachain_api_, claim_queue(hash))
          .WillRepeatedly(Return(std::nullopt));
      std::vector<runtime::CoreState> cores;
      for (const auto &[_, paras] : test_state.claim_queue) {
        cores.emplace_back(runtime::ScheduledCore{
            .para_id = paras[0],
            .collator = std::nullopt,
        });
      }
      EXPECT_CALL(*parachain_api_, availability_cores(hash))
          .WillRepeatedly(Return(outcome::success(cores)));
    } else {
      EXPECT_CALL(*parachain_api_, claim_queue(hash))
          .WillRepeatedly(Return(ClaimQueueSnapshot{test_state.claim_queue}));
    }

    EXPECT_CALL(*block_tree_, getBlockHeader(hash))
        .WillRepeatedly(Return(header));

    const BlockNumber min_min = [&, number = number]() -> BlockNumber {
      std::optional<BlockNumber> min_min;
      for (const auto &[_, data] : leaf.para_data) {
        min_min = (min_min ? std::min(*min_min, data.min_relay_parent)
                           : data.min_relay_parent);
      }
      if (min_min) {
        return *min_min;
      }
      return number;
    }();

    const auto ancestry_len = number - min_min;
    std::vector<Hash> ancestry_hashes;
    std::vector<BlockNumber> ancestry_numbers;

    Hash d = hash;
    for (BlockNumber x = 0; x <= ancestry_len; ++x) {
      ancestry_hashes.emplace_back(d);
      ancestry_numbers.push_back(number - x);
      d = func(d);
    }
    ASSERT_EQ(ancestry_hashes.size(), ancestry_numbers.size());

    if (ancestry_len > 0) {
      EXPECT_CALL(*block_tree_,
                  getDescendingChainToBlock(hash, ALLOWED_ANCESTRY_LEN + 1))
          .WillRepeatedly(Return(ancestry_hashes));
      EXPECT_CALL(*parachain_api_, session_index_for_child(hash))
          .WillRepeatedly(Return(1));
    }

    std::unordered_set<Hash> used_relay_parents;
    for (size_t i = 0; i < ancestry_hashes.size(); ++i) {
      const auto &h_ = ancestry_hashes[i];
      const auto &n_ = ancestry_numbers[i];

      if (!used_relay_parents.contains(h_)) {
        BlockHeader h{
            .number = n_,
            .parent_hash = get_parent_hash(h_),
            .state_root = {},
            .extrinsics_root = {},
            .digest = {},
            .hash_opt = {},
        };
        EXPECT_CALL(*block_tree_, getBlockHeader(h_)).WillRepeatedly(Return(h));
        EXPECT_CALL(*parachain_api_, session_index_for_child(h_))
            .WillRepeatedly(Return(outcome::success(1)));
        used_relay_parents.emplace(h_);
      }
    }

    std::unordered_set<ParachainId> paras;
    for (const auto &[_, values] : test_state.claim_queue) {
      for (const auto &value : values) {
        paras.emplace(value);
      }
    }

    for (auto it = paras.begin(); it != paras.end(); ++it) {
      const auto para_id = *it;
      const auto &[min_relay_parent, head_data, pending_availability] =
          leaf.paraData(para_id).get();

      const auto constraints =
          dummy_constraints(min_relay_parent,
                            {number},
                            head_data,
                            test_state.validation_code_hash);
      const fragment::BackingState backing_state{
          .constraints = constraints,
          .pending_availability = pending_availability,
      };
      EXPECT_CALL(*parachain_api_, staging_para_backing_state(hash, para_id))
          .WillRepeatedly(Return(backing_state));

      for (const auto &pending : pending_availability) {
        if (!used_relay_parents.contains(pending.descriptor.relay_parent)) {
          BlockHeader h{
              .number = pending.relay_parent_number,
              .parent_hash = get_parent_hash(pending.descriptor.relay_parent),
              .state_root = {},
              .extrinsics_root = {},
              .digest = {},
              .hash_opt = {},
          };
          EXPECT_CALL(*block_tree_,
                      getBlockHeader(pending.descriptor.relay_parent))
              .WillRepeatedly(Return(h));
          used_relay_parents.emplace(pending.descriptor.relay_parent);
        }
      }
    }

    ASSERT_OUTCOME_SUCCESS_TRY(
        prospective_parachain_->onActiveLeavesUpdate(network::ExViewRef{
            .new_head = {update.new_head},
            .lost = update.lost,
        }));

    auto resp = prospective_parachain_->answerMinimumRelayParentsRequest(hash);
    std::sort(resp.begin(), resp.end(), [](const auto &l, const auto &r) {
      return l.first < r.first;
    });

    std::vector<std::pair<ParachainId, BlockNumber>> mrp_response;
    for (const auto &[pid, ppd] : para_data) {
      mrp_response.emplace_back(pid, ppd.min_relay_parent);
    }
    ASSERT_EQ(resp, mrp_response);
  }

  void deactivate_leaf(const Hash &hash) {
    std::vector<Hash> lost = {hash};
    ASSERT_OUTCOME_SUCCESS_TRY(
        prospective_parachain_->onActiveLeavesUpdate(network::ExViewRef{
            .new_head = {},
            .lost = lost,
        }));
  }

  std::pair<network::CommittedCandidateReceipt, CandidateHash>
  make_and_back_candidate(const TestState &test_state,
                          const TestLeaf &leaf,
                          const network::CommittedCandidateReceipt &parent,
                          uint64_t index) {
    auto [candidate, pvd] = make_candidate(leaf.hash,
                                           leaf.number,
                                           1,
                                           parent.commitments.para_head,
                                           {uint8_t(index)},
                                           test_state.validation_code_hash);

    candidate.descriptor.para_head_hash = fromNumber(index);
    const Hash candidate_hash = network::candidateHash(*hasher_, candidate);

    introduce_seconded_candidate(candidate, pvd);
    back_candidate(candidate, candidate_hash);

    return {candidate, candidate_hash};
  }

  void activate_leaf(
      const TestLeaf &leaf,
      const TestState &test_state,
      const fragment::AsyncBackingParams &async_backing_params,
      std::function<Hash(const Hash &)> parent_hash_fn = get_parent_hash) {
    const auto &[number, hash, para_data] = leaf;
    BlockHeader header{
        .number = number,
        .parent_hash = get_parent_hash(hash),
        .state_root = {},
        .extrinsics_root = {},
        .digest = {},
        .hash_opt = {},
    };

    network::ExView update{
        .view = {},
        .new_head = header,
        .lost = {},
    };
    update.new_head.hash_opt = hash;
    handle_leaf_activation_2(update,
                             leaf,
                             test_state,
                             async_backing_params,
                             std::move(parent_hash_fn));
  }

  runtime::PersistedValidationData dummy_pvd(const HeadData &parent_head,
                                             uint32_t relay_parent_number) {
    return runtime::PersistedValidationData{
        .parent_head = parent_head,
        .relay_parent_number = relay_parent_number,
        .relay_parent_storage_root = {},
        .max_pov_size = MAX_POV_SIZE,
    };
  }

  network::CandidateCommitments dummy_candidate_commitments(
      const std::optional<HeadData> &head_data) {
    return network::CandidateCommitments{
        .upward_msgs = {},
        .outbound_hor_msgs = {},
        .opt_para_runtime = std::nullopt,
        .para_head = (head_data ? *head_data : dummy_head_data()),
        .downward_msgs_count = 0,
        .watermark = 0,
    };
  }

  /// Create meaningless validation code.
  runtime::ValidationCode dummy_validation_code() {
    return {1, 2, 3, 4, 5, 6, 7, 8, 9};
  }

  network::CandidateDescriptor dummy_candidate_descriptor_bad_sig(
      const Hash &relay_parent) {
    return network::CandidateDescriptor{
        .para_id = 0,
        .relay_parent = relay_parent,
        .reserved_1 = {},
        .persisted_data_hash = fromNumber(0),
        .pov_hash = fromNumber(0),
        .erasure_encoding_root = fromNumber(0),
        .reserved_2 = {},
        .para_head_hash = fromNumber(0),
        .validation_code_hash =
            crypto::Hashed<runtime::ValidationCode,
                           32,
                           crypto::Blake2b_StreamHasher<32>>(
                dummy_validation_code())
                .getHash(),
    };
  }

  HeadData dummy_head_data() {
    return {};
  }

  network::CandidateReceipt dummy_candidate_receipt_bad_sig(
      const Hash &relay_parent, const std::optional<Hash> &commitments) {
    const auto commitments_hash = [&]() -> Hash {
      if (commitments) {
        return *commitments;
      }
      return crypto::Hashed<network::CandidateCommitments,
                            32,
                            crypto::Blake2b_StreamHasher<32>>(
                 dummy_candidate_commitments(dummy_head_data()))
          .getHash();
    }();

    network::CandidateReceipt receipt;
    receipt.descriptor = dummy_candidate_descriptor_bad_sig(relay_parent);
    receipt.commitments_hash = commitments_hash;
    return receipt;
  }

  auto get_backable_candidates(
      const TestLeaf &leaf,
      ParachainId para_id,
      fragment::Ancestors ancestors,
      uint32_t count,
      const std::vector<std::pair<CandidateHash, Hash>> &expected_result) {
    auto resp = prospective_parachain_->answerGetBackableCandidates(
        leaf.hash, para_id, count, ancestors);
    ASSERT_EQ(resp, expected_result);
  }

  void back_candidate(const network::CommittedCandidateReceipt &candidate,
                      const CandidateHash &candidate_hash) {
    prospective_parachain_->candidate_backed(candidate.descriptor.para_id,
                                             candidate_hash);
  }

  void introduce_seconded_candidate(
      const network::CommittedCandidateReceipt &candidate,
      const runtime::PersistedValidationData &pvd) {
    prospective_parachain_->introduce_seconded_candidate(
        candidate.descriptor.para_id,
        candidate,
        crypto::Hashed<runtime::PersistedValidationData,
                       32,
                       crypto::Blake2b_StreamHasher<32>>(pvd),
        network::candidateHash(*hasher_, candidate));
  }

  void introduce_seconded_candidate_failed(
      const network::CommittedCandidateReceipt &candidate,
      const runtime::PersistedValidationData &pvd) {
    auto res = prospective_parachain_->introduce_seconded_candidate(
        candidate.descriptor.para_id, candidate, pvd, hash(candidate));

    ASSERT_TRUE(!res);
  }

  Hash hash(const network::CommittedCandidateReceipt &value) {
    return network::candidateHash(*hasher_, value);
  }

  void get_hypothetical_membership(
      const CandidateHash &candidate_hash,
      const network::CommittedCandidateReceipt &receipt,
      const runtime::PersistedValidationData &persisted_validation_data,
      const std::vector<Hash> &expected_membership) {
    const HypotheticalCandidateComplete hypothetical_candidate{
        .candidate_hash = candidate_hash,
        .receipt = receipt,
        .persisted_validation_data = persisted_validation_data,
    };

    std::vector<HypotheticalCandidate> candidates = {hypothetical_candidate};
    const auto resp =
        prospective_parachain_->answer_hypothetical_membership_request(
            candidates, std::nullopt);
    ASSERT_EQ(resp.size(), 1);

    const auto &[candidate, membership] = resp[0];
    ASSERT_TRUE(candidate == HypotheticalCandidate(hypothetical_candidate));
    ASSERT_EQ((std::unordered_set<Hash>(membership.begin(), membership.end())),
              (std::unordered_set<Hash>(expected_membership.begin(),
                                        expected_membership.end())));
  }

  auto get_pvd(
      ParachainId para_id,
      const Hash &candidate_relay_parent,
      const HeadData &parent_head_data,
      const std::optional<runtime::PersistedValidationData> &expected_pvd) {
    auto resp = prospective_parachain_->answerProspectiveValidationDataRequest(
        candidate_relay_parent,
        ParentHeadData_OnlyHash(hasher_->blake2b_256(parent_head_data)),
        para_id);
    ASSERT_TRUE(resp.has_value());
    ASSERT_EQ(resp.value(), expected_pvd);
  }

  std::pair<network::CommittedCandidateReceipt,
            runtime::PersistedValidationData>
  make_candidate(const Hash &relay_parent_hash,
                 BlockNumber relay_parent_number,
                 ParachainId para_id,
                 const HeadData &parent_head,
                 const HeadData &head_data,
                 const ValidationCodeHash &validation_code_hash) {
    const runtime::PersistedValidationData pvd =
        dummy_pvd(parent_head, relay_parent_number);
    network::CandidateCommitments commitments{
        .upward_msgs = {},
        .outbound_hor_msgs = {},
        .opt_para_runtime = std::nullopt,
        .para_head = head_data,
        .downward_msgs_count = 0,
        .watermark = relay_parent_number,
    };

    auto candidate = dummy_candidate_receipt_bad_sig(relay_parent_hash, Hash{});
    candidate.commitments_hash =
        crypto::Hashed<network::CandidateCommitments,
                       32,
                       crypto::Blake2b_StreamHasher<32>>(commitments)
            .getHash();
    candidate.descriptor.para_id = para_id;
    candidate.descriptor.persisted_data_hash =
        crypto::Hashed<runtime::PersistedValidationData,
                       32,
                       crypto::Blake2b_StreamHasher<32>>(pvd)
            .getHash();
    candidate.descriptor.validation_code_hash = validation_code_hash;
    return std::make_pair(
        network::CommittedCandidateReceipt{
            .descriptor = candidate.descriptor,
            .commitments = commitments,
        },
        pvd);
  }
};

TEST_F(ProspectiveParachainsTest,
       should_do_no_work_if_async_backing_disabled_for_leaf) {
  const auto hash = fromNumber(130);
  network::ExView update{
      .view = {},
      .new_head =
          BlockHeader{
              .number = 1,
              .parent_hash = get_parent_hash(hash),
              .state_root = {},
              .extrinsics_root = {},
              .digest = {},
              .hash_opt = {},
          },
      .lost = {},
  };
  update.new_head.hash_opt = hash;

  EXPECT_CALL(*parachain_api_, staging_async_backing_params(hash))
      .WillRepeatedly(
          Return(outcome::failure(ParachainProcessor::Error::NO_STATE)));

  std::ignore = prospective_parachain_->onActiveLeavesUpdate(network::ExViewRef{
      .new_head = {update.new_head},
      .lost = update.lost,
  });
  ASSERT_TRUE(prospective_parachain_->view().active_leaves.empty());
}

TEST_F(ProspectiveParachainsTest, introduce_candidates_basic) {
  TestState test_state;

  const ParachainId chain_a(1);
  const ParachainId chain_b(2);
  test_state.claim_queue = {{CoreIndex(0), {chain_a, chain_b}}};

  // Leaf A
  const TestLeaf leaf_a{
      .number = 100,
      .hash = fromNumber(130),
      .para_data =
          {
              {1, PerParaData(97, {1, 2, 3})},
              {2, PerParaData(100, {2, 3, 4})},
          },
  };
  // Leaf B
  const TestLeaf leaf_b{
      .number = 101,
      .hash = fromNumber(131),
      .para_data =
          {
              {1, PerParaData(99, {3, 4, 5})},
              {2, PerParaData(101, {4, 5, 6})},
          },
  };
  // Leaf C
  const TestLeaf leaf_c{
      .number = 102,
      .hash = fromNumber(132),
      .para_data =
          {
              {1, PerParaData(102, {5, 6, 7})},
              {2, PerParaData(98, {6, 7, 8})},
          },
  };

  // Activate leaves.
  activate_leaf(leaf_a, test_state, ASYNC_BACKING_PARAMETERS);
  activate_leaf(leaf_b, test_state, ASYNC_BACKING_PARAMETERS);
  activate_leaf(leaf_c, test_state, ASYNC_BACKING_PARAMETERS);

  // Candidate A1

  const auto &[candidate_a1, pvd_a1] =
      make_candidate(leaf_a.hash,
                     leaf_a.number,
                     1,
                     {1, 2, 3},
                     {1},
                     test_state.validation_code_hash);

  const Hash candidate_hash_a1 = network::candidateHash(*hasher_, candidate_a1);
  std::vector<std::pair<CandidateHash, Hash>> response_a1 = {
      {candidate_hash_a1, leaf_a.hash}};

  // Candidate A2
  const auto &[candidate_a2, pvd_a2] =
      make_candidate(leaf_a.hash,
                     leaf_a.number,
                     2,
                     {2, 3, 4},
                     {2},
                     test_state.validation_code_hash);
  const auto candidate_hash_a2 = network::candidateHash(*hasher_, candidate_a2);
  std::vector<std::pair<CandidateHash, Hash>> response_a2 = {
      {candidate_hash_a2, leaf_a.hash}};

  // Candidate B
  const auto &[candidate_b, pvd_b] =
      make_candidate(leaf_b.hash,
                     leaf_b.number,
                     1,
                     {3, 4, 5},
                     {3},
                     test_state.validation_code_hash);
  const auto candidate_hash_b = network::candidateHash(*hasher_, candidate_b);
  std::vector<std::pair<CandidateHash, Hash>> response_b = {
      {candidate_hash_b, leaf_b.hash}};

  // Candidate C
  const auto &[candidate_c, pvd_c] =
      make_candidate(leaf_c.hash,
                     leaf_c.number,
                     2,
                     {6, 7, 8},
                     {4},
                     test_state.validation_code_hash);
  const auto candidate_hash_c = network::candidateHash(*hasher_, candidate_c);
  std::vector<std::pair<CandidateHash, Hash>> response_c = {
      {candidate_hash_c, leaf_c.hash}};

  // Introduce candidates.
  introduce_seconded_candidate(candidate_a1, pvd_a1);
  introduce_seconded_candidate(candidate_a2, pvd_a2);
  introduce_seconded_candidate(candidate_b, pvd_b);
  introduce_seconded_candidate(candidate_c, pvd_c);

  // Back candidates. Otherwise, we cannot check membership with
  // GetBackableCandidates.
  back_candidate(candidate_a1, candidate_hash_a1);
  back_candidate(candidate_a2, candidate_hash_a2);
  back_candidate(candidate_b, candidate_hash_b);
  back_candidate(candidate_c, candidate_hash_c);

  // Check candidate tree membership.
  get_backable_candidates(leaf_a, 1, {}, 5, response_a1);
  get_backable_candidates(leaf_a, 2, {}, 5, response_a2);
  get_backable_candidates(leaf_b, 1, {}, 5, response_b);
  get_backable_candidates(leaf_c, 2, {}, 5, response_c);

  // Check membership on other leaves.
  get_backable_candidates(leaf_b, 2, {}, 5, {});
  get_backable_candidates(leaf_c, 1, {}, 5, {});

  ASSERT_EQ(prospective_parachain_->view().active_leaves.size(), 3);
}

TEST_F(ProspectiveParachainsTest, introduce_candidate_multiple_times) {
  TestState test_state;

  // Leaf A
  const TestLeaf leaf_a{
      .number = 100,
      .hash = fromNumber(130),
      .para_data =
          {
              {1, PerParaData(97, {1, 2, 3})},
              {2, PerParaData(100, {2, 3, 4})},
          },
  };
  activate_leaf(leaf_a, test_state, ASYNC_BACKING_PARAMETERS);

  // Candidate A.
  const auto &[candidate_a, pvd_a] =
      make_candidate(leaf_a.hash,
                     leaf_a.number,
                     1,
                     {1, 2, 3},
                     {1},
                     test_state.validation_code_hash);
  const auto candidate_hash_a = network::candidateHash(*hasher_, candidate_a);
  std::vector<std::pair<CandidateHash, Hash>> response_a = {
      {candidate_hash_a, leaf_a.hash}};

  // Introduce candidates.
  introduce_seconded_candidate(candidate_a, pvd_a);

  // Back candidates. Otherwise, we cannot check membership with
  // GetBackableCandidates.
  back_candidate(candidate_a, candidate_hash_a);

  // Check candidate tree membership.
  get_backable_candidates(leaf_a, 1, {}, 5, response_a);

  // Introduce the same candidate multiple times. It'll return true but it will
  // only be added once.
  for (size_t i = 0; i < 5; ++i) {
    introduce_seconded_candidate(candidate_a, pvd_a);
  }

  // Check candidate tree membership.
  get_backable_candidates(leaf_a, 1, {}, 5, response_a);
  ASSERT_EQ(prospective_parachain_->view().active_leaves.size(), 1);
}

TEST_F(ProspectiveParachainsTest, fragment_chain_best_chain_length_is_bounded) {
  TestState test_state;

  // Leaf A
  const TestLeaf leaf_a{
      .number = 100,
      .hash = fromNumber(130),
      .para_data =
          {
              {1, PerParaData(97, {1, 2, 3})},
              {2, PerParaData(100, {2, 3, 4})},
          },
  };
  activate_leaf(leaf_a,
                test_state,
                fragment::AsyncBackingParams{
                    .max_candidate_depth = 1,
                    .allowed_ancestry_len = 3,
                });

  // Candidates A, B and C form a chain.
  const auto &[candidate_a, pvd_a] =
      make_candidate(leaf_a.hash,
                     leaf_a.number,
                     1,
                     {1, 2, 3},
                     {1},
                     test_state.validation_code_hash);
  const auto &[candidate_b, pvd_b] = make_candidate(
      leaf_a.hash, leaf_a.number, 1, {1}, {2}, test_state.validation_code_hash);
  const auto &[candidate_c, pvd_c] = make_candidate(
      leaf_a.hash, leaf_a.number, 1, {2}, {3}, test_state.validation_code_hash);

  // Introduce candidates A and B. Since max depth is 1, only these two will be
  // allowed.
  introduce_seconded_candidate(candidate_a, pvd_a);
  introduce_seconded_candidate(candidate_b, pvd_b);

  // Back candidates. Otherwise, we cannot check membership with
  // GetBackableCandidates and they won't be part of the best chain.
  back_candidate(candidate_a, network::candidateHash(*hasher_, candidate_a));
  back_candidate(candidate_b, network::candidateHash(*hasher_, candidate_b));

  // Check candidate tree membership.
  get_backable_candidates(
      leaf_a,
      1,
      {},
      5,
      {{network::candidateHash(*hasher_, candidate_a), leaf_a.hash},
       {network::candidateHash(*hasher_, candidate_b), leaf_a.hash}});

  // Introducing C will not fail. It will be kept as unconnected storage.
  introduce_seconded_candidate(candidate_c, pvd_c);
  // When being backed, candidate C will be dropped.
  back_candidate(candidate_c, network::candidateHash(*hasher_, candidate_c));

  get_backable_candidates(
      leaf_a,
      1,
      {},
      5,
      {{network::candidateHash(*hasher_, candidate_a), leaf_a.hash},
       {network::candidateHash(*hasher_, candidate_b), leaf_a.hash}});
  ASSERT_EQ(prospective_parachain_->view().active_leaves.size(), 1);
}

TEST_F(ProspectiveParachainsTest, introduce_candidate_parent_leaving_view) {
  TestState test_state;

  // Leaf A
  const TestLeaf leaf_a{
      .number = 100,
      .hash = fromNumber(130),
      .para_data =
          {
              {1, PerParaData(97, {1, 2, 3})},
              {2, PerParaData(100, {2, 3, 4})},
          },
  };
  // Leaf B
  const TestLeaf leaf_b{
      .number = 101,
      .hash = fromNumber(131),
      .para_data =
          {
              {1, PerParaData(99, {3, 4, 5})},
              {2, PerParaData(101, {4, 5, 6})},
          },
  };
  // Leaf C
  const TestLeaf leaf_c{
      .number = 102,
      .hash = fromNumber(132),
      .para_data =
          {
              {1, PerParaData(102, {5, 6, 7})},
              {2, PerParaData(98, {6, 7, 8})},
          },
  };

  // Activate leaves.
  activate_leaf(leaf_a, test_state, ASYNC_BACKING_PARAMETERS);
  activate_leaf(leaf_b, test_state, ASYNC_BACKING_PARAMETERS);
  activate_leaf(leaf_c, test_state, ASYNC_BACKING_PARAMETERS);

  // Candidate A1
  const auto &[candidate_a1, pvd_a1] =
      make_candidate(leaf_a.hash,
                     leaf_a.number,
                     1,
                     {1, 2, 3},
                     {1},
                     test_state.validation_code_hash);
  const auto candidate_hash_a1 = network::candidateHash(*hasher_, candidate_a1);

  // Candidate A2
  const auto &[candidate_a2, pvd_a2] =
      make_candidate(leaf_a.hash,
                     leaf_a.number,
                     2,
                     {2, 3, 4},
                     {2},
                     test_state.validation_code_hash);
  const auto candidate_hash_a2 = network::candidateHash(*hasher_, candidate_a2);

  // Candidate B
  const auto &[candidate_b, pvd_b] =
      make_candidate(leaf_b.hash,
                     leaf_b.number,
                     1,
                     {3, 4, 5},
                     {3},
                     test_state.validation_code_hash);
  const auto candidate_hash_b = network::candidateHash(*hasher_, candidate_b);
  std::vector<std::pair<CandidateHash, Hash>> response_b = {
      {candidate_hash_b, leaf_b.hash}};

  // Candidate C
  const auto &[candidate_c, pvd_c] =
      make_candidate(leaf_c.hash,
                     leaf_c.number,
                     2,
                     {6, 7, 8},
                     {4},
                     test_state.validation_code_hash);
  const auto candidate_hash_c = network::candidateHash(*hasher_, candidate_c);
  std::vector<std::pair<CandidateHash, Hash>> response_c = {
      {candidate_hash_c, leaf_c.hash}};

  // Introduce candidates.
  introduce_seconded_candidate(candidate_a1, pvd_a1);
  introduce_seconded_candidate(candidate_a2, pvd_a2);
  introduce_seconded_candidate(candidate_b, pvd_b);
  introduce_seconded_candidate(candidate_c, pvd_c);

  // Back candidates. Otherwise, we cannot check membership with
  // GetBackableCandidates.
  back_candidate(candidate_a1, candidate_hash_a1);
  back_candidate(candidate_a2, candidate_hash_a2);
  back_candidate(candidate_b, candidate_hash_b);
  back_candidate(candidate_c, candidate_hash_c);

  // Deactivate leaf A.
  deactivate_leaf(leaf_a.hash);

  // Candidates A1 and A2 should be gone. Candidates B and C should remain.
  get_backable_candidates(leaf_a, 1, {}, 5, {});
  get_backable_candidates(leaf_a, 2, {}, 5, {});
  get_backable_candidates(leaf_b, 1, {}, 5, response_b);
  get_backable_candidates(leaf_c, 2, {}, 5, response_c);

  // Deactivate leaf B.
  deactivate_leaf(leaf_b.hash);

  // Candidate B should be gone, C should remain.
  get_backable_candidates(leaf_a, 1, {}, 5, {});
  get_backable_candidates(leaf_a, 2, {}, 5, {});
  get_backable_candidates(leaf_b, 1, {}, 5, {});
  get_backable_candidates(leaf_c, 2, {}, 5, response_c);

  // Deactivate leaf C.
  deactivate_leaf(leaf_c.hash);

  // Candidate C should be gone.
  get_backable_candidates(leaf_a, 1, {}, 5, {});
  get_backable_candidates(leaf_a, 2, {}, 5, {});
  get_backable_candidates(leaf_b, 1, {}, 5, {});
  get_backable_candidates(leaf_c, 2, {}, 5, {});

  ASSERT_EQ(prospective_parachain_->view().active_leaves.size(), 0);
}

TEST_F(ProspectiveParachainsTest, introduce_candidate_on_multiple_forks) {
  TestState test_state;

  // Leaf B
  const TestLeaf leaf_b{
      .number = 101,
      .hash = fromNumber(131),
      .para_data =
          {
              {1, PerParaData(99, {1, 2, 3})},
              {2, PerParaData(101, {4, 5, 6})},
          },
  };
  // Leaf A
  const TestLeaf leaf_a{
      .number = 100,
      .hash = get_parent_hash(leaf_b.hash),
      .para_data =
          {
              {1, PerParaData(97, {1, 2, 3})},
              {2, PerParaData(100, {2, 3, 4})},
          },
  };

  // Activate leaves.
  activate_leaf(leaf_a, test_state, ASYNC_BACKING_PARAMETERS);
  activate_leaf(leaf_b, test_state, ASYNC_BACKING_PARAMETERS);

  // Candidate built on leaf A.
  const auto &[candidate_a, pvd_a] =
      make_candidate(leaf_a.hash,
                     leaf_a.number,
                     1,
                     {1, 2, 3},
                     {1},
                     test_state.validation_code_hash);
  const auto candidate_hash_a = network::candidateHash(*hasher_, candidate_a);
  std::vector<std::pair<CandidateHash, Hash>> response_a = {
      {candidate_hash_a, leaf_a.hash}};

  // Introduce candidate. Should be present on leaves B and C.
  introduce_seconded_candidate(candidate_a, pvd_a);
  back_candidate(candidate_a, candidate_hash_a);

  // Check candidate tree membership.
  get_backable_candidates(leaf_a, 1, {}, 5, response_a);
  get_backable_candidates(leaf_b, 1, {}, 5, response_a);

  ASSERT_EQ(prospective_parachain_->view().active_leaves.size(), 2);
}

TEST_F(ProspectiveParachainsTest, unconnected_candidates_become_connected) {
  TestState test_state;

  // Leaf A
  const TestLeaf leaf_a{
      .number = 100,
      .hash = fromNumber(130),
      .para_data =
          {
              {1, PerParaData(97, {1, 2, 3})},
              {2, PerParaData(100, {2, 3, 4})},
          },
  };

  // Activate leaves.
  activate_leaf(leaf_a, test_state, ASYNC_BACKING_PARAMETERS);

  // Candidates A, B, C and D all form a chain, but we'll first introduce A, C
  // and D.
  const auto &[candidate_a, pvd_a] =
      make_candidate(leaf_a.hash,
                     leaf_a.number,
                     1,
                     {1, 2, 3},
                     {1},
                     test_state.validation_code_hash);
  const auto &[candidate_b, pvd_b] = make_candidate(
      leaf_a.hash, leaf_a.number, 1, {1}, {2}, test_state.validation_code_hash);
  const auto &[candidate_c, pvd_c] = make_candidate(
      leaf_a.hash, leaf_a.number, 1, {2}, {3}, test_state.validation_code_hash);
  const auto &[candidate_d, pvd_d] = make_candidate(
      leaf_a.hash, leaf_a.number, 1, {3}, {4}, test_state.validation_code_hash);

  // Introduce candidates A, C and D.
  introduce_seconded_candidate(candidate_a, pvd_a);
  introduce_seconded_candidate(candidate_c, pvd_c);
  introduce_seconded_candidate(candidate_d, pvd_d);

  // Back candidates. Otherwise, we cannot check membership with
  // GetBackableCandidates.
  back_candidate(candidate_a, network::candidateHash(*hasher_, candidate_a));
  back_candidate(candidate_c, network::candidateHash(*hasher_, candidate_c));
  back_candidate(candidate_d, network::candidateHash(*hasher_, candidate_d));

  // Check candidate tree membership. Only A should be returned.
  get_backable_candidates(
      leaf_a,
      1,
      {},
      5,
      {{network::candidateHash(*hasher_, candidate_a), leaf_a.hash}});

  // Introduce C and check membership. Full chain should be returned.
  introduce_seconded_candidate(candidate_b, pvd_b);
  back_candidate(candidate_b, network::candidateHash(*hasher_, candidate_b));
  get_backable_candidates(
      leaf_a,
      1,
      {},
      5,
      {{network::candidateHash(*hasher_, candidate_a), leaf_a.hash},
       {network::candidateHash(*hasher_, candidate_b), leaf_a.hash},
       {network::candidateHash(*hasher_, candidate_c), leaf_a.hash},
       {network::candidateHash(*hasher_, candidate_d), leaf_a.hash}});

  ASSERT_EQ(prospective_parachain_->view().active_leaves.size(), 1);
}

TEST_F(ProspectiveParachainsTest, check_backable_query_single_candidate) {
  TestState test_state;

  // Leaf A
  const TestLeaf leaf_a{
      .number = 100,
      .hash = fromNumber(130),
      .para_data =
          {
              {1, PerParaData(97, {1, 2, 3})},
              {2, PerParaData(100, {2, 3, 4})},
          },
  };

  // Activate leaves.
  activate_leaf(leaf_a, test_state, ASYNC_BACKING_PARAMETERS);

  // Candidate A
  const auto &[candidate_a, pvd_a] =
      make_candidate(leaf_a.hash,
                     leaf_a.number,
                     1,
                     {1, 2, 3},
                     {1},
                     test_state.validation_code_hash);
  const auto candidate_hash_a = network::candidateHash(*hasher_, candidate_a);

  // Candidate B
  auto [candidate_b, pvd_b] = make_candidate(
      leaf_a.hash, leaf_a.number, 1, {1}, {2}, test_state.validation_code_hash);

  // Set a field to make this candidate unique.
  candidate_b.descriptor.para_head_hash = fromNumber(254);
  const auto candidate_hash_b = hash(candidate_b);

  // Introduce candidates.
  introduce_seconded_candidate(candidate_a, pvd_a);
  introduce_seconded_candidate(candidate_b, pvd_b);

  // Should not get any backable candidates.
  get_backable_candidates(leaf_a, 1, {candidate_hash_a}, 1, {});
  get_backable_candidates(leaf_a, 1, {candidate_hash_a}, 0, {});
  get_backable_candidates(leaf_a, 1, {}, 0, {});

  // Back candidates.
  back_candidate(candidate_a, candidate_hash_a);
  back_candidate(candidate_b, candidate_hash_b);

  // Back an unknown candidate. It doesn't return anything but it's ignored.
  // Will not have any effect on the backable candidates.
  back_candidate(candidate_b, fromNumber(200));

  // Should not get any backable candidates for the other para.
  get_backable_candidates(leaf_a, 2, {}, 1, {});
  get_backable_candidates(leaf_a, 2, {candidate_hash_a}, 1, {});

  // Get backable candidate.
  get_backable_candidates(leaf_a, 1, {}, 1, {{candidate_hash_a, leaf_a.hash}});
  get_backable_candidates(
      leaf_a, 1, {candidate_hash_a}, 1, {{candidate_hash_b, leaf_a.hash}});

  // Wrong path
  get_backable_candidates(
      leaf_a, 1, {candidate_hash_b}, 1, {{candidate_hash_a, leaf_a.hash}});

  ASSERT_EQ(prospective_parachain_->view().active_leaves.size(), 1);
}

TEST_F(ProspectiveParachainsTest, check_backable_query_multiple_candidates) {
  TestState test_state;

  // Leaf A
  const TestLeaf leaf_a{
      .number = 100,
      .hash = fromNumber(130),
      .para_data =
          {
              {1, PerParaData(97, {1, 2, 3})},
              {2, PerParaData(100, {2, 3, 4})},
          },
  };

  // Activate leaves.
  activate_leaf(leaf_a, test_state, ASYNC_BACKING_PARAMETERS);

  // Candidate A
  const auto &[candidate_a, pvd_a] =
      make_candidate(leaf_a.hash,
                     leaf_a.number,
                     1,
                     {1, 2, 3},
                     {1},
                     test_state.validation_code_hash);
  const auto candidate_hash_a = hash(candidate_a);
  introduce_seconded_candidate(candidate_a, pvd_a);
  back_candidate(candidate_a, candidate_hash_a);

  auto [candidate_b, candidate_hash_b] =
      make_and_back_candidate(test_state, leaf_a, candidate_a, 2);
  auto [candidate_c, candidate_hash_c] =
      make_and_back_candidate(test_state, leaf_a, candidate_b, 3);
  auto [_candidate_d, candidate_hash_d] =
      make_and_back_candidate(test_state, leaf_a, candidate_c, 4);

  // Should not get any backable candidates for the other para.
  get_backable_candidates(leaf_a, 2, {}, 1, {});
  get_backable_candidates(leaf_a, 2, {}, 5, {});
  get_backable_candidates(leaf_a, 2, {candidate_hash_a}, 1, {});

  // Test various scenarios with various counts.

  // empty ancestors
  {
    get_backable_candidates(
        leaf_a, 1, {}, 1, {{candidate_hash_a, leaf_a.hash}});
    for (size_t count = 4; count < 10; ++count) {
      get_backable_candidates(leaf_a,
                              1,
                              {},
                              count,
                              {{candidate_hash_a, leaf_a.hash},
                               {candidate_hash_b, leaf_a.hash},
                               {candidate_hash_c, leaf_a.hash},
                               {candidate_hash_d, leaf_a.hash}});
    }
  }

  // ancestors of size 1
  {
    get_backable_candidates(
        leaf_a, 1, {candidate_hash_a}, 1, {{candidate_hash_b, leaf_a.hash}});
    get_backable_candidates(
        leaf_a,
        1,
        {candidate_hash_a},
        2,
        {{candidate_hash_b, leaf_a.hash}, {candidate_hash_c, leaf_a.hash}});

    // If the requested count exceeds the largest chain, return the longest
    // chain we can get.
    for (size_t count = 3; count < 10; ++count) {
      get_backable_candidates(leaf_a,
                              1,
                              {candidate_hash_a},
                              count,
                              {{candidate_hash_b, leaf_a.hash},
                               {candidate_hash_c, leaf_a.hash},
                               {candidate_hash_d, leaf_a.hash}});
    }
  }

  // ancestor count 2 and higher
  {
    get_backable_candidates(
        leaf_a,
        1,
        {candidate_hash_a, candidate_hash_b, candidate_hash_c},
        1,
        {{candidate_hash_d, leaf_a.hash}});

    get_backable_candidates(leaf_a,
                            1,
                            {candidate_hash_a, candidate_hash_b},
                            1,
                            {{candidate_hash_c, leaf_a.hash}});

    // If the requested count exceeds the largest chain, return the longest
    // chain we can get.
    for (size_t count = 3; count < 10; ++count) {
      get_backable_candidates(
          leaf_a,
          1,
          {candidate_hash_a, candidate_hash_b},
          count,
          {{candidate_hash_c, leaf_a.hash}, {candidate_hash_d, leaf_a.hash}});
    }
  }

  // No more candidates in the chain.
  for (size_t count = 1; count < 4; ++count) {
    get_backable_candidates(leaf_a,
                            1,
                            {candidate_hash_a,
                             candidate_hash_b,
                             candidate_hash_c,
                             candidate_hash_d},
                            count,
                            {});
  }

  // Wrong paths.
  get_backable_candidates(
      leaf_a, 1, {candidate_hash_b}, 1, {{candidate_hash_a, leaf_a.hash}});
  get_backable_candidates(leaf_a,
                          1,
                          {candidate_hash_b, candidate_hash_c},
                          3,
                          {{candidate_hash_a, leaf_a.hash},
                           {candidate_hash_b, leaf_a.hash},
                           {candidate_hash_c, leaf_a.hash}});

  get_backable_candidates(
      leaf_a,
      1,
      {candidate_hash_a, candidate_hash_c, candidate_hash_d},
      2,
      {{candidate_hash_b, leaf_a.hash}, {candidate_hash_c, leaf_a.hash}});

  // Non-existent candidate.
  get_backable_candidates(
      leaf_a,
      1,
      {candidate_hash_a, fromNumber(100)},
      2,
      {{candidate_hash_b, leaf_a.hash}, {candidate_hash_c, leaf_a.hash}});

  // Requested count is zero.
  get_backable_candidates(leaf_a, 1, {}, 0, {});
  get_backable_candidates(leaf_a, 1, {candidate_hash_a}, 0, {});
  get_backable_candidates(
      leaf_a, 1, {candidate_hash_a, candidate_hash_b}, 0, {});

  ASSERT_EQ(prospective_parachain_->view().active_leaves.size(), 1);
}

TEST_F(ProspectiveParachainsTest, check_hypothetical_membership_query) {
  TestState test_state;

  const TestLeaf leaf_b{
      .number = 101,
      .hash = fromNumber(131),
      .para_data =
          {
              {1, PerParaData(97, {1, 2, 3})},
              {2, PerParaData(100, {2, 3, 4})},
          },
  };

  const TestLeaf leaf_a{
      .number = 100,
      .hash = get_parent_hash(leaf_b.hash),
      .para_data =
          {
              {1, PerParaData(98, {1, 2, 3})},
              {2, PerParaData(100, {2, 3, 4})},
          },
  };

  // Activate leaves.
  activate_leaf(leaf_a,
                test_state,
                fragment::AsyncBackingParams{
                    .max_candidate_depth = 1,
                    .allowed_ancestry_len = 3,
                });
  activate_leaf(leaf_b,
                test_state,
                fragment::AsyncBackingParams{
                    .max_candidate_depth = 1,
                    .allowed_ancestry_len = 3,
                });

  // Candidates will be valid on both leaves.

  // Candidate A.
  auto [candidate_a, pvd_a] = make_candidate(leaf_a.hash,
                                             leaf_a.number,
                                             1,
                                             {1, 2, 3},
                                             {1},
                                             test_state.validation_code_hash);

  // Candidate B.
  auto [candidate_b, pvd_b] = make_candidate(
      leaf_a.hash, leaf_a.number, 1, {1}, {2}, test_state.validation_code_hash);

  // Candidate C.
  auto [candidate_c, pvd_c] = make_candidate(
      leaf_a.hash, leaf_a.number, 1, {2}, {3}, test_state.validation_code_hash);

  // Get hypothetical membership of candidates before adding candidate A.
  // Candidate A can be added directly, candidates B and C are potential
  // candidates.
  for (const auto &[candidate, pvd] : {std::make_pair(candidate_a, pvd_a),
                                       std::make_pair(candidate_b, pvd_b),
                                       std::make_pair(candidate_c, pvd_c)}) {
    get_hypothetical_membership(
        hash(candidate), candidate, pvd, {leaf_a.hash, leaf_b.hash});
  }

  // Add candidate A.
  introduce_seconded_candidate(candidate_a, pvd_a);

  // Get membership of candidates after adding A. They all are still unconnected
  // candidates (not part of the best backable chain).
  for (const auto &[candidate, pvd] : {std::make_pair(candidate_a, pvd_a),
                                       std::make_pair(candidate_b, pvd_b),
                                       std::make_pair(candidate_c, pvd_c)}) {
    get_hypothetical_membership(
        hash(candidate), candidate, pvd, {leaf_a.hash, leaf_b.hash});
  }

  // Back A. Now A is part of the best chain the rest can be added as
  // unconnected.

  back_candidate(candidate_a, hash(candidate_a));

  for (const auto &[candidate, pvd] : {std::make_pair(candidate_a, pvd_a),
                                       std::make_pair(candidate_b, pvd_b),
                                       std::make_pair(candidate_c, pvd_c)}) {
    get_hypothetical_membership(
        hash(candidate), candidate, pvd, {leaf_a.hash, leaf_b.hash});
  }

  // Candidate D has invalid relay parent.
  const auto [candidate_d, pvd_d] =
      make_candidate(fromNumber(200),
                     leaf_a.number,
                     1,
                     {1},
                     {2},
                     test_state.validation_code_hash);
  introduce_seconded_candidate_failed(candidate_d, pvd_d);

  // Add candidate B and back it.
  introduce_seconded_candidate(candidate_b, pvd_b);
  back_candidate(candidate_b, hash(candidate_b));

  // Get membership of candidates after adding B.
  for (const auto &[candidate, pvd] : {std::make_pair(candidate_a, pvd_a),
                                       std::make_pair(candidate_b, pvd_b),
                                       std::make_pair(candidate_c, pvd_c)}) {
    get_hypothetical_membership(
        hash(candidate), candidate, pvd, {leaf_a.hash, leaf_b.hash});
  }

  ASSERT_EQ(prospective_parachain_->view().active_leaves.size(), 2);
}

TEST_F(ProspectiveParachainsTest, check_pvd_query) {
  TestState test_state;

  // Leaf A
  const TestLeaf leaf_a{
      .number = 100,
      .hash = fromNumber(130),
      .para_data =
          {
              {1, PerParaData(97, {1, 2, 3})},
              {2, PerParaData(100, {2, 3, 4})},
          },
  };

  // Activate leaves.
  activate_leaf(leaf_a, test_state, ASYNC_BACKING_PARAMETERS);

  // Candidate A.
  const auto [candidate_a, pvd_a] =
      make_candidate(leaf_a.hash,
                     leaf_a.number,
                     1,
                     {1, 2, 3},
                     {1},
                     test_state.validation_code_hash);

  // Candidate B.
  const auto [candidate_b, pvd_b] = make_candidate(
      leaf_a.hash, leaf_a.number, 1, {1}, {2}, test_state.validation_code_hash);

  // Candidate C.
  const auto [candidate_c, pvd_c] = make_candidate(
      leaf_a.hash, leaf_a.number, 1, {2}, {3}, test_state.validation_code_hash);

  // Candidate E.
  const auto [candidate_e, pvd_e] = make_candidate(
      leaf_a.hash, leaf_a.number, 1, {5}, {6}, test_state.validation_code_hash);

  // Get pvd of candidate A before adding it.
  get_pvd(1, leaf_a.hash, {1, 2, 3}, pvd_a);

  // Add candidate A.
  introduce_seconded_candidate(candidate_a, pvd_a);
  back_candidate(candidate_a, hash(candidate_a));

  // Get pvd of candidate A after adding it.
  get_pvd(1, leaf_a.hash, {1, 2, 3}, pvd_a);

  // Get pvd of candidate B before adding it.
  get_pvd(1, leaf_a.hash, {1}, pvd_b);

  // Add candidate B.
  introduce_seconded_candidate(candidate_b, pvd_b);

  // Get pvd of candidate B after adding it.
  get_pvd(1, leaf_a.hash, {1}, pvd_b);

  // Get pvd of candidate C before adding it.
  get_pvd(1, leaf_a.hash, {2}, pvd_c);

  // Add candidate C.
  introduce_seconded_candidate(candidate_c, pvd_c);

  // Get pvd of candidate C after adding it.
  get_pvd(1, leaf_a.hash, {2}, pvd_c);

  // Get pvd of candidate E before adding it. It won't be found, as we don't
  // have its parent.
  get_pvd(1, leaf_a.hash, {5}, std::nullopt);

  // Add candidate E and check again. Should succeed this time.
  introduce_seconded_candidate(candidate_e, pvd_e);

  get_pvd(1, leaf_a.hash, {5}, pvd_e);

  ASSERT_EQ(prospective_parachain_->view().active_leaves.size(), 1);
}

TEST_F(ProspectiveParachainsTest, correctly_updates_leaves) {
  TestState test_state;
  test_state.enable_claim_queue_api = false;

  // Leaf A
  const TestLeaf leaf_a{
      .number = 100,
      .hash = fromNumber(130),
      .para_data =
          {
              {1, PerParaData(97, {1, 2, 3})},
              {2, PerParaData(100, {2, 3, 4})},
          },
  };
  // Leaf B
  const TestLeaf leaf_b{
      .number = 101,
      .hash = fromNumber(131),
      .para_data =
          {
              {1, PerParaData(99, {3, 4, 5})},
              {2, PerParaData(101, {4, 5, 6})},
          },
  };
  // Leaf C
  const TestLeaf leaf_c{
      .number = 102,
      .hash = fromNumber(132),
      .para_data =
          {
              {1, PerParaData(102, {5, 6, 7})},
              {2, PerParaData(98, {6, 7, 8})},
          },
  };

  // Activate leaves.
  activate_leaf(leaf_a, test_state, ASYNC_BACKING_PARAMETERS);
  activate_leaf(leaf_b, test_state, ASYNC_BACKING_PARAMETERS);

  // Try activating a duplicate leaf.
  activate_leaf(leaf_b, test_state, ASYNC_BACKING_PARAMETERS);

  // Pass in an empty update.
  ASSERT_OUTCOME_SUCCESS_TRY(
      prospective_parachain_->onActiveLeavesUpdate(network::ExViewRef{
          .new_head = std::nullopt,
          .lost = {},
      }));

  // Activate a leaf and remove one at the same time.
  {
    network::ExView update{
        .view = {},
        .new_head =
            BlockHeader{
                .number = leaf_c.number,
                .parent_hash = get_parent_hash(leaf_c.hash),
                .state_root = {},
                .extrinsics_root = {},
                .digest = {},
                .hash_opt = {},
            },
        .lost = {leaf_b.hash},
    };

    update.new_head.hash_opt = leaf_c.hash;
    // ASSERT_OUTCOME_SUCCESS_TRY(
    //     prospective_parachain_->onActiveLeavesUpdate(network::ExViewRef{
    //         .new_head = {update.new_head},
    //         .lost = update.lost,
    //     }));
    handle_leaf_activation_2(
        update, leaf_c, test_state, ASYNC_BACKING_PARAMETERS);
  }

  // Remove all remaining leaves.
  ASSERT_OUTCOME_SUCCESS_TRY(
      prospective_parachain_->onActiveLeavesUpdate(network::ExViewRef{
          .new_head = {},
          .lost = {leaf_a.hash, leaf_c.hash},
      }));

  // Activate and deactivate the same leaf.
  {
    network::ExView update{
        .view = {},
        .new_head =
            BlockHeader{
                .number = leaf_a.number,
                .parent_hash = get_parent_hash(leaf_a.hash),
                .state_root = {},
                .extrinsics_root = {},
                .digest = {},
                .hash_opt = {},
            },
        .lost = {leaf_a.hash},
    };

    update.new_head.hash_opt = leaf_a.hash;
    ASSERT_OUTCOME_SUCCESS_TRY(
        prospective_parachain_->onActiveLeavesUpdate(network::ExViewRef{
            .new_head = {update.new_head},
            .lost = update.lost,
        }));
  }

  // Remove the leaf again. Send some unnecessary hashes.
  {
    ASSERT_OUTCOME_SUCCESS_TRY(
        prospective_parachain_->onActiveLeavesUpdate(network::ExViewRef{
            .new_head = {},
            .lost = {leaf_a.hash, leaf_b.hash, leaf_c.hash},
        }));
  }

  ASSERT_EQ(prospective_parachain_->view().active_leaves.size(), 0);
}

TEST_F(ProspectiveParachainsTest,
       handle_active_leaves_update_gets_candidates_from_parent) {
  const ParachainId para_id(1);
  TestState test_state;

  ClaimQueue claim_queue;
  for (const auto &[i, paras] : test_state.claim_queue) {
    if (paras[0] == para_id) {
      claim_queue[i] = paras;
    }
  }
  test_state.claim_queue = std::move(claim_queue);
  ASSERT_EQ(test_state.claim_queue.size(), 1);

  // Leaf A
  const TestLeaf leaf_a{
      .number = 100,
      .hash = fromNumber(130),
      .para_data =
          {
              {para_id, PerParaData(97, {1, 2, 3})},
          },
  };

  // Activate leaf A.
  activate_leaf(leaf_a, test_state, ASYNC_BACKING_PARAMETERS);

  // Candidates A, B, C and D all form a chain
  const auto [candidate_a, pvd_a] =
      make_candidate(leaf_a.hash,
                     leaf_a.number,
                     para_id,
                     {1, 2, 3},
                     {1},
                     test_state.validation_code_hash);
  const auto candidate_hash_a = hash(candidate_a);
  introduce_seconded_candidate(candidate_a, pvd_a);
  back_candidate(candidate_a, candidate_hash_a);

  const auto [candidate_b, candidate_hash_b] =
      make_and_back_candidate(test_state, leaf_a, candidate_a, 2);
  const auto [candidate_c, candidate_hash_c] =
      make_and_back_candidate(test_state, leaf_a, candidate_b, 3);
  const auto [candidate_d, candidate_hash_d] =
      make_and_back_candidate(test_state, leaf_a, candidate_c, 4);

  std::vector<std::pair<CandidateHash, Hash>> all_candidates_resp = {
      {candidate_hash_a, leaf_a.hash},
      {candidate_hash_b, leaf_a.hash},
      {candidate_hash_c, leaf_a.hash},
      {candidate_hash_d, leaf_a.hash}};

  // Check candidate tree membership.
  get_backable_candidates(leaf_a, para_id, {}, 5, all_candidates_resp);

  // Activate leaf B, which makes candidates A and B pending availability.
  // Leaf B
  const TestLeaf leaf_b{
      .number = 101,
      .hash = fromNumber(129),
      .para_data = {{
          para_id,
          PerParaData(98,
                      {1, 2, 3},
                      {
                          fragment::CandidatePendingAvailability{
                              .candidate_hash = hash(candidate_a),
                              .descriptor = candidate_a.descriptor,
                              .commitments = candidate_a.commitments,
                              .relay_parent_number = leaf_a.number,
                              .max_pov_size = MAX_POV_SIZE,
                          },
                          fragment::CandidatePendingAvailability{
                              .candidate_hash = hash(candidate_b),
                              .descriptor = candidate_b.descriptor,
                              .commitments = candidate_b.commitments,
                              .relay_parent_number = leaf_a.number,
                              .max_pov_size = MAX_POV_SIZE,
                          },
                      }),
      }},
  };
  // Activate leaf B.
  activate_leaf(leaf_b, test_state, ASYNC_BACKING_PARAMETERS);
  get_backable_candidates(leaf_b, para_id, {}, 5, {});

  get_backable_candidates(
      leaf_b,
      para_id,
      {hash(candidate_a), hash(candidate_b)},
      5,
      {{hash(candidate_c), leaf_a.hash}, {hash(candidate_d), leaf_a.hash}});

  get_backable_candidates(leaf_b, para_id, {}, 5, {});

  get_backable_candidates(leaf_a, para_id, {}, 5, all_candidates_resp);

  // Now deactivate leaf A.
  deactivate_leaf(leaf_a.hash);

  get_backable_candidates(leaf_b, para_id, {}, 5, {});

  get_backable_candidates(
      leaf_b,
      para_id,
      {hash(candidate_a), hash(candidate_b)},
      5,
      {{hash(candidate_c), leaf_a.hash}, {hash(candidate_d), leaf_a.hash}});

  // Now add leaf C, which will be a sibling (fork) of leaf B. It should also
  // inherit the candidates of leaf A (their common parent).
  const TestLeaf leaf_c{
      .number = 101,
      .hash = fromNumber(12),
      .para_data =
          {
              {para_id, PerParaData(98, {1, 2, 3}, {})},
          },
  };

  activate_leaf(
      leaf_c, test_state, ASYNC_BACKING_PARAMETERS, [&](const auto &hash) {
        if (hash == leaf_c.hash) {
          return leaf_a.hash;
        }
        return get_parent_hash(hash);
      });

  get_backable_candidates(
      leaf_b,
      para_id,
      {hash(candidate_a), hash(candidate_b)},
      5,
      {{hash(candidate_c), leaf_a.hash}, {hash(candidate_d), leaf_a.hash}});

  get_backable_candidates(leaf_c, para_id, {}, 5, all_candidates_resp);

  // Deactivate C and add another candidate that will be present on the
  // deactivated parent A. When activating C again it should also get the new
  // candidate. Deactivated leaves are still updated with new candidates.
  deactivate_leaf(leaf_c.hash);

  const auto [candidate_e, _] =
      make_and_back_candidate(test_state, leaf_a, candidate_d, 5);
  activate_leaf(
      leaf_c, test_state, ASYNC_BACKING_PARAMETERS, [&](const auto &hash) {
        if (hash == leaf_c.hash) {
          return leaf_a.hash;
        }
        return get_parent_hash(hash);
      });

  get_backable_candidates(leaf_b,
                          para_id,
                          {hash(candidate_a), hash(candidate_b)},
                          5,
                          {{hash(candidate_c), leaf_a.hash},
                           {hash(candidate_d), leaf_a.hash},
                           {hash(candidate_e), leaf_a.hash}});

  all_candidates_resp.emplace_back(hash(candidate_e), leaf_a.hash);
  get_backable_candidates(leaf_c, para_id, {}, 5, all_candidates_resp);

  // Querying the backable candidates for deactivated leaf won't work.
  get_backable_candidates(leaf_a, para_id, {}, 5, {});

  ASSERT_EQ(prospective_parachain_->view().active_leaves.size(), 2);
  ASSERT_EQ(prospective_parachain_->view().per_relay_parent.size(), 3);
}

TEST_F(ProspectiveParachainsTest,
       handle_active_leaves_update_bounded_implicit_view) {
  const ParachainId para_id(1);
  TestState test_state;

  ClaimQueue claim_queue;
  for (const auto &[i, paras] : test_state.claim_queue) {
    if (paras[0] == para_id) {
      claim_queue[i] = paras;
    }
  }
  test_state.claim_queue = std::move(claim_queue);
  ASSERT_EQ(test_state.claim_queue.size(), 1);

  std::vector<TestLeaf> leaves_ = {TestLeaf{
      .number = 100,
      .hash = fromNumber(130),
      .para_data =
          {
              {para_id, PerParaData(100 - ALLOWED_ANCESTRY_LEN, {1, 2, 3})},
          },
  }};

  for (size_t index = 1; index < 10; ++index) {
    const auto &prev_leaf = leaves_[index - 1];
    leaves_.emplace_back(TestLeaf{
        .number = prev_leaf.number - 1,
        .hash = get_parent_hash(prev_leaf.hash),
        .para_data = {{
            para_id,
            PerParaData(prev_leaf.number - 1 - ALLOWED_ANCESTRY_LEN, {1, 2, 3}),
        }},
    });
  }

  std::vector<TestLeaf> leaves;
  for (const auto &i : leaves_ | std::views::reverse) {
    leaves.emplace_back(i);
  }

  // Activate first 10 leaves.
  for (const auto &leaf : leaves) {
    activate_leaf(leaf, test_state, ASYNC_BACKING_PARAMETERS);
  }

  // Now deactivate first 9 leaves.
  for (size_t i = 0; i < 9; ++i) {
    const auto &leaf = leaves[i];
    deactivate_leaf(leaf.hash);
  }

  // Only latest leaf is active.
  ASSERT_EQ(prospective_parachain_->view().active_leaves.size(), 1);

  // We keep allowed_ancestry_len implicit leaves. The latest leaf is also
  // present here.
  ASSERT_EQ(prospective_parachain_->view().per_relay_parent.size(),
            ASYNC_BACKING_PARAMETERS.allowed_ancestry_len + 1);
  ASSERT_EQ(*prospective_parachain_->view().active_leaves.begin(),
            leaves[9].hash);

  std::unordered_set<Hash> l;
  for (const auto &[h, _] : prospective_parachain_->view().per_relay_parent) {
    l.insert(h);
  }

  std::unordered_set<Hash> r;
  for (size_t i = 6; i < leaves.size(); ++i) {
    r.insert(leaves[i].hash);
  }
  ASSERT_EQ(l, r);
}

TEST_F(ProspectiveParachainsTest, persists_pending_availability_candidate) {
  const ParachainId para_id(1);
  TestState test_state;

  ClaimQueue claim_queue;
  for (const auto &[i, paras] : test_state.claim_queue) {
    if (paras[0] == para_id) {
      claim_queue[i] = paras;
    }
  }
  test_state.claim_queue = std::move(claim_queue);
  ASSERT_EQ(test_state.claim_queue.size(), 1);

  const HeadData para_head = {1, 2, 3};
  // Min allowed relay parent for leaf `a` which goes out of scope in the test.
  const auto candidate_relay_parent = fromNumber(5);
  const BlockNumber candidate_relay_parent_number = 97;

  const TestLeaf leaf_a{
      .number = candidate_relay_parent_number + ALLOWED_ANCESTRY_LEN,
      .hash = fromNumber(2),
      .para_data =
          {
              {para_id, PerParaData(candidate_relay_parent_number, para_head)},
          },
  };

  const auto leaf_b_hash = fromNumber(1);
  const BlockNumber leaf_b_number = leaf_a.number + 1;

  // Activate leaf.
  activate_leaf(leaf_a, test_state, ASYNC_BACKING_PARAMETERS);

  // Candidate A
  const auto [candidate_a, pvd_a] =
      make_candidate(candidate_relay_parent,
                     candidate_relay_parent_number,
                     para_id,
                     para_head,
                     {1},
                     test_state.validation_code_hash);
  const auto candidate_hash_a = hash(candidate_a);

  // Candidate B, built on top of the candidate which is out of scope but
  // pending availability.
  const auto [candidate_b, pvd_b] =
      make_candidate(leaf_b_hash,
                     leaf_b_number,
                     para_id,
                     {1},
                     {2},
                     test_state.validation_code_hash);
  const auto candidate_hash_b = hash(candidate_b);

  introduce_seconded_candidate(candidate_a, pvd_a);
  back_candidate(candidate_a, candidate_hash_a);

  const fragment::CandidatePendingAvailability candidate_a_pending_av{
      .candidate_hash = candidate_hash_a,
      .descriptor = candidate_a.descriptor,
      .commitments = candidate_a.commitments,
      .relay_parent_number = candidate_relay_parent_number,
      .max_pov_size = MAX_POV_SIZE,
  };
  const TestLeaf leaf_b{
      .number = leaf_b_number,
      .hash = leaf_b_hash,
      .para_data =
          {
              {1,
               PerParaData(candidate_relay_parent_number + 1,
                           para_head,
                           {candidate_a_pending_av})},
          },
  };

  activate_leaf(leaf_b, test_state, ASYNC_BACKING_PARAMETERS);

  get_hypothetical_membership(
      candidate_hash_a, candidate_a, pvd_a, {leaf_a.hash, leaf_b.hash});

  introduce_seconded_candidate(candidate_b, pvd_b);
  back_candidate(candidate_b, candidate_hash_b);

  get_backable_candidates(leaf_b,
                          para_id,
                          {candidate_hash_a},
                          1,
                          {{candidate_hash_b, leaf_b_hash}});
}

TEST_F(ProspectiveParachainsTest,
       backwards_compatible_with_non_async_backing_params) {
  const ParachainId para_id(1);
  TestState test_state;

  ClaimQueue claim_queue;
  for (const auto &[i, paras] : test_state.claim_queue) {
    if (paras[0] == para_id) {
      claim_queue[i] = paras;
    }
  }
  test_state.claim_queue = std::move(claim_queue);
  ASSERT_EQ(test_state.claim_queue.size(), 1);

  const HeadData para_head = {1, 2, 3};

  const auto leaf_b_hash = fromNumber(15);
  const auto candidate_relay_parent = get_parent_hash(leaf_b_hash);
  const BlockNumber candidate_relay_parent_number = 100;

  const TestLeaf leaf_a{
      .number = candidate_relay_parent_number,
      .hash = candidate_relay_parent,
      .para_data = {{para_id,
                     PerParaData(candidate_relay_parent_number, para_head)}},
  };

  // Activate leaf.
  activate_leaf(leaf_a,
                test_state,
                fragment::AsyncBackingParams{
                    .max_candidate_depth = 0,
                    .allowed_ancestry_len = 0,
                });

  // Candidate A
  const auto [candidate_a, pvd_a] =
      make_candidate(candidate_relay_parent,
                     candidate_relay_parent_number,
                     para_id,
                     para_head,
                     {1},
                     test_state.validation_code_hash);
  const auto candidate_hash_a = hash(candidate_a);

  introduce_seconded_candidate(candidate_a, pvd_a);
  back_candidate(candidate_a, candidate_hash_a);

  get_backable_candidates(
      leaf_a, para_id, {}, 1, {{candidate_hash_a, candidate_relay_parent}});

  const TestLeaf leaf_b{
      .number = candidate_relay_parent_number + 1,
      .hash = leaf_b_hash,
      .para_data = {
          {para_id,
           PerParaData(candidate_relay_parent_number + 1, para_head)}}};
  activate_leaf(leaf_b,
                test_state,
                fragment::AsyncBackingParams{
                    .max_candidate_depth = 0,
                    .allowed_ancestry_len = 0,
                });

  get_backable_candidates(leaf_b, para_id, {}, 1, {});
}

TEST_F(ProspectiveParachainsTest, uses_ancestry_only_within_session) {
  const BlockNumber number = 5;
  const auto hash = fromNumber(5);
  const size_t ancestry_len = 3;
  const size_t session = 2;

  std::vector<Hash> ancestry_hashes = {
      fromNumber(4), fromNumber(3), fromNumber(2)};
  const auto session_change_hash = fromNumber(3);

  std::vector<Hash> ancestry_hashes_long = {
      hash, fromNumber(4), fromNumber(3), fromNumber(2)};

  // let activated = new_leaf(hash, number);
  //
  // virtual_overseer
  //	.send(FromOrchestra::Signal(OverseerSignal::ActiveLeaves(
  //		ActiveLeavesUpdate::start_work(activated),
  //	)))
  //	.await;

  EXPECT_CALL(*parachain_api_, staging_async_backing_params(hash))
      .WillRepeatedly(Return(outcome::success(fragment::AsyncBackingParams{
          .max_candidate_depth = 0, .allowed_ancestry_len = ancestry_len})));

  EXPECT_CALL(*parachain_api_, claim_queue(hash))
      .WillRepeatedly(Return(ClaimQueueSnapshot{}));

  EXPECT_CALL(*block_tree_, getBlockHeader(hash))
      .WillRepeatedly(Return(BlockHeader{
          .number = number,
          .parent_hash = get_parent_hash(hash),
          .state_root = {},
          .extrinsics_root = {},
          .digest = {},
          .hash_opt = {},
      }));

  EXPECT_CALL(*block_tree_, getDescendingChainToBlock(hash, ancestry_len + 1))
      .WillRepeatedly(Return(ancestry_hashes_long));

  EXPECT_CALL(*parachain_api_, session_index_for_child(hash))
      .WillRepeatedly(Return(session));

  for (size_t i = 0; i < ancestry_hashes.size(); ++i) {
    const auto &h = ancestry_hashes[i];
    const BlockNumber n = number - (i + 1);
    EXPECT_CALL(*block_tree_, getBlockHeader(h))
        .WillRepeatedly(Return(BlockHeader{
            .number = n,
            .parent_hash = get_parent_hash(h),
            .state_root = {},
            .extrinsics_root = {},
            .digest = {},
            .hash_opt = {},
        }));

    SessionIndex s;
    if (h == session_change_hash) {
      s = session - 1;
    } else {
      s = session;
    }

    EXPECT_CALL(*parachain_api_, session_index_for_child(h))
        .WillRepeatedly(Return(s));
  }

  const BlockHeader header{
      .number = number,
      .parent_hash = get_parent_hash(hash),
      .state_root = {},
      .extrinsics_root = {},
      .digest = {},
      .hash_opt = {},
  };
  network::ExView update{
      .view = {},
      .new_head = header,
      .lost = {},
  };
  update.new_head.hash_opt = hash;

  ASSERT_OUTCOME_SUCCESS_TRY(
      prospective_parachain_->onActiveLeavesUpdate(network::ExViewRef{
          .new_head = {update.new_head},
          .lost = update.lost,
      }));
}
