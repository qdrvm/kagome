/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "parachain/validator/prospective_parachains/prospective_parachains.hpp"
#include <map>
#include <vector>
#include "core/parachain/parachain_test_harness.hpp"
#include "parachain/validator/parachain_processor.hpp"

using namespace kagome::parachain;
namespace runtime = kagome::runtime;

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
    uint32_t runtime_api_version = CLAIM_QUEUE_RUNTIME_REQUIREMENT;
    ValidationCodeHash validation_code_hash = fromNumber(42);

    void set_runtime_api_version(uint32_t version) {
      runtime_api_version = version;
    }
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

  static Hash get_parent_hash(const Hash &parent) {
    const auto val = *(uint8_t *)&parent[0];
    return fromNumber(val + 1);
  }

  void handle_leaf_activation_2(
      const network::ExView &update,
      const TestLeaf &leaf,
      const TestState &test_state,
      const fragment::AsyncBackingParams &async_backing_params) {
    const auto &[number, hash, para_data] = leaf;
    const auto &header = update.new_head;

    EXPECT_CALL(*parachain_api_, staging_async_backing_params(hash))
        .WillRepeatedly(Return(outcome::success(async_backing_params)));

    EXPECT_CALL(*parachain_api_, runtime_api_version(hash))
        .WillRepeatedly(
            Return(outcome::success(test_state.runtime_api_version)));

    if (test_state.runtime_api_version < CLAIM_QUEUE_RUNTIME_REQUIREMENT) {
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
          .WillRepeatedly(Return(outcome::success(test_state.claim_queue)));
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
      d = get_parent_hash(d);
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

  void activate_leaf(const TestLeaf &leaf,
                     const TestState &test_state,
                     const fragment::AsyncBackingParams &async_backing_params) {
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
    handle_leaf_activation_2(update, leaf, test_state, async_backing_params);
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
        .collator_id = {},
        .persisted_data_hash = fromNumber(0),
        .pov_hash = fromNumber(0),
        .erasure_encoding_root = fromNumber(0),
        .signature = {},
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
          Return(outcome::failure(ParachainProcessorImpl::Error::NO_STATE)));

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
