///**
// * Copyright Quadrivium LLC
// * All Rights Reserved
// * SPDX-License-Identifier: Apache-2.0
// */

//#include <gtest/gtest.h>
//
//#include "testutil/literals.hpp"
//#include "testutil/outcome.hpp"
//#include "testutil/prepare_loggers.hpp"
//
//#include "crypto/hasher/hasher_impl.hpp"
//#include "crypto/random_generator/boost_generator.hpp"
//#include "crypto/sr25519/sr25519_provider_impl.hpp"
//#include "crypto/type_hasher.hpp"
//#include "mock/core/blockchain/block_tree_mock.hpp"
//#include "mock/core/runtime/parachain_host_mock.hpp"
//#include "parachain/types.hpp"
//#include "parachain/validator/impl/candidates.hpp"
//#include "parachain/validator/signer.hpp"
//#include "runtime/runtime_api/parachain_host_types.hpp"
//#include "scale/kagome_scale.hpp"
//#include "scale/scale.hpp"
//#include "testutil/scale_test_comparator.hpp"
//
//using namespace kagome::primitives;
//using namespace kagome::parachain;
//
//namespace network = kagome::network;
//namespace runtime = kagome::runtime;
//namespace common = kagome::common;
//namespace crypto = kagome::crypto;
//
//using testing::Return;
//

/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "parachain/validator/prospective_parachains/fragment_chain.hpp"
#include <map>
#include <random>

#include "core/parachain/parachain_test_harness.hpp"
#include "parachain/validator/prospective_parachains/prospective_parachains.hpp"
#include "parachain/validator/parachain_processor.hpp"

using namespace kagome::parachain::fragment;

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

struct TestState {
  std::vector<runtime::CoreState> availability_cores;
  std::map<CoreIndex, std::vector<ParachainId>> claim_queue;
  ValidationCodeHash validation_code_hash;

  TestState(const std::shared_ptr<kagome::crypto::Hasher> &hasher)
      : availability_cores{{runtime::ScheduledCore{.para_id = ParachainId{1},
                                                   .collator = std::nullopt},
                            runtime::ScheduledCore{.para_id = ParachainId{2},
                                                   .collator = std::nullopt}}},
        validation_code_hash{ghashFromStrData(hasher, "42")} {}

  ParachainId byIndex(size_t ix) const {
    assert(ix < availability_cores.size());
    const runtime::CoreState &cs = availability_cores[ix];
    if (const runtime::ScheduledCore *ptr =
            std::get_if<runtime::ScheduledCore>(&cs)) {
      return ptr->para_id;
    }
    UNREACHABLE;
  }
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

class ProspectiveParachainsTest : public ProspectiveParachainsTestHarness {
 protected:
  static kagome::log::Logger log() {
    static auto logger = kagome::log::createLogger("test");
    return logger;
  }

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

  std::shared_ptr<runtime::ParachainHostMock> parachain_api_;
  std::shared_ptr<ProspectiveParachains> prospective_parachain_;

  Hash get_parent_hash(const Hash &parent) const {
    Hash h{};
    *(uint64_t *)&h[0] = *(uint64_t *)&parent[0] + 1ull;
    return h;
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

    EXPECT_CALL(*parachain_api_, availability_cores(hash))
        .WillRepeatedly(
            Return(outcome::success(test_state.availability_cores)));

    EXPECT_CALL(*block_tree_, getBlockHeader(hash))
        .WillRepeatedly(Return(header));

    BlockNumber min_min = [&, number = number]() -> BlockNumber {
      std::optional<BlockNumber> min_min;
      for (const auto &[_, data] : leaf.para_data) {
        min_min = min_min ? std::min(*min_min, data.min_relay_parent)
                          : data.min_relay_parent;
      }
      if (min_min) {
        return *min_min;
      }
      return number;
    }();
    const auto ancestry_len = number - min_min;
    std::vector<Hash> ancestry_hashes;
    std::deque<BlockNumber> ancestry_numbers;

    Hash d = hash;
    for (BlockNumber x = 0; x <= ancestry_len + 1; ++x) {
      assert(number - x - 1 != 0);
      ancestry_hashes.emplace_back(d);
      ancestry_numbers.push_front(number - ancestry_len + x - 1);
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

    for (size_t i = 0; i < ancestry_hashes.size(); ++i) {
      const auto &h_ = ancestry_hashes[i];
      const auto &n_ = ancestry_numbers[i];

      ASSERT_TRUE(n_ > 0);
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
    }

    for (size_t i = 0; i < test_state.availability_cores.size(); ++i) {
      const auto para_id = test_state.byIndex(i);
      const auto &[min_relay_parent, head_data, pending_availability] =
          leaf.paraData(para_id).get();
      fragment::BackingState backing_state{
          .constraints = dummy_constraints(min_relay_parent,
                                           {number},
                                           head_data,
                                           test_state.validation_code_hash),
          .pending_availability = pending_availability,
      };
      EXPECT_CALL(*parachain_api_, staging_para_backing_state(hash, para_id))
          .WillRepeatedly(Return(backing_state));

      for (const auto &pending : pending_availability) {
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

  void handle_leaf_activation(
      const TestLeaf &leaf,
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

  void activate_leaf(const TestLeaf &leaf,
                     const TestState &test_state,
                     const fragment::AsyncBackingParams &async_backing_params) {
    handle_leaf_activation(leaf, test_state, async_backing_params);
  }

  fragment::Constraints dummy_constraints(
      BlockNumber min_relay_parent_number,
      std::vector<BlockNumber> valid_watermarks,
      const HeadData &required_parent,
      const ValidationCodeHash &validation_code_hash) {
    return fragment::Constraints{
        .min_relay_parent_number = min_relay_parent_number,
        .max_pov_size = 5 * 1024 * 1024,
        .max_code_size = 1000 * 1000,
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

  std::pair<network::CommittedCandidateReceipt,
            runtime::PersistedValidationData>
  make_candidate(const Hash &relay_parent_hash,
                 BlockNumber relay_parent_number,
                 ParachainId para_id,
                 const HeadData &parent_head,
                 const HeadData &head_data,
                 const ValidationCodeHash &validation_code_hash) {
    runtime::PersistedValidationData pvd{
        .parent_head = parent_head,
        .relay_parent_number = relay_parent_number,
        .relay_parent_storage_root = {},
        .max_pov_size = 1'000'000,
    };

    network::CandidateCommitments commitments{
        .upward_msgs = {},
        .outbound_hor_msgs = {},
        .opt_para_runtime = std::nullopt,
        .para_head = head_data,
        .downward_msgs_count = 0,
        .watermark = relay_parent_number,
    };

    network::CandidateReceipt candidate{};
    candidate.descriptor = network::CandidateDescriptor{
        .para_id = 0,
        .relay_parent = relay_parent_hash,
        .collator_id = {},
        .persisted_data_hash = {},
        .pov_hash = {},
        .erasure_encoding_root = {},
        .signature = {},
        .para_head_hash = {},
        .validation_code_hash =
            hasher_->blake2b_256(std::vector<uint8_t>{1, 2, 3}),
    };
    candidate.commitments_hash = {};

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

  void introduce_seconded_candidate(const network::CommittedCandidateReceipt &candidate,
                           const runtime::PersistedValidationData &pvd) {
    ASSERT_TRUE(prospective_parachain_->introduce_seconded_candidate(
        candidate.descriptor.para_id,
        candidate,
        crypto::Hashed<runtime::PersistedValidationData,
                       32,
                       crypto::Blake2b_StreamHasher<32>>(pvd),
        hash(candidate)));
  }

};


//class ProspectiveParachainsTest : public testing::Test {
//  void SetUp() override {
//    testutil::prepareLoggers();
//    hasher_ = std::make_shared<kagome::crypto::HasherImpl>();
//
//    parachain_api_ = std::make_shared<runtime::ParachainHostMock>();
//    block_tree_ = std::make_shared<kagome::blockchain::BlockTreeMock>();
//    prospective_parachain_ = std::make_shared<ProspectiveParachains>(
//        hasher_, parachain_api_, block_tree_);
//    sr25519_provider_ = std::make_shared<crypto::Sr25519ProviderImpl>();
//  }
//
//  void TearDown() override {
//    prospective_parachain_.reset();
//    block_tree_.reset();
//    parachain_api_.reset();
//  }
//
// protected:
//  using CandidatesHashMap = std::unordered_map<
//      Hash,
//      std::unordered_map<ParachainId, std::unordered_set<CandidateHash>>>;
//
//  std::shared_ptr<kagome::crypto::Hasher> hasher_;
//  std::shared_ptr<runtime::ParachainHostMock> parachain_api_;
//  std::shared_ptr<kagome::blockchain::BlockTreeMock> block_tree_;
//  std::shared_ptr<ProspectiveParachains> prospective_parachain_;
//  std::shared_ptr<crypto::Sr25519Provider> sr25519_provider_;
//
//  static constexpr uint64_t ALLOWED_ANCESTRY_LEN = 3ull;
//  static constexpr uint32_t MAX_POV_SIZE = 1000000;
//
//  Hash hashFromStrData(std::span<const char> data) {
//    return ghashFromStrData(hasher_, data);
//  }
//
//  fragment::Constraints make_constraints(
//      BlockNumber min_relay_parent_number,
//      std::vector<BlockNumber> valid_watermarks,
//      HeadData required_parent) {
//    return fragment::Constraints{
//        .min_relay_parent_number = min_relay_parent_number,
//        .max_pov_size = 1000000,
//        .max_code_size = 1000000,
//        .ump_remaining = 10,
//        .ump_remaining_bytes = 1000,
//        .max_ump_num_per_candidate = 10,
//        .dmp_remaining_messages = std::vector<BlockNumber>(10, 0),
//        .hrmp_inbound = fragment::InboundHrmpLimitations{.valid_watermarks =
//                                                             valid_watermarks},
//        .hrmp_channels_out = {},
//        .max_hrmp_num_per_candidate = 0,
//        .required_parent = required_parent,
//        .validation_code_hash = hashFromStrData("42"),
//        .upgrade_restriction = std::nullopt,
//        .future_validation_code = std::nullopt,
//    };
//  }
//
//  std::pair<network::CommittedCandidateReceipt, CandidateHash>
//  make_and_back_candidate(const TestState &test_state,
//                          const TestLeaf &leaf,
//                          const network::CommittedCandidateReceipt &parent,
//                          uint64_t index) {
//    auto tmp = make_candidate(leaf.hash,
//                              leaf.number,
//                              1,
//                              parent.commitments.para_head,
//                              {uint8_t(index)},
//                              test_state.validation_code_hash);
//
//    tmp.first.descriptor.para_head_hash = fromNumber(index);
//    const auto &[candidate, pvd] = tmp;
//    const Hash candidate_hash = network::candidateHash(*hasher_, candidate);
//
//    introduce_candidate(candidate, pvd);
//    second_candidate(candidate);
//    back_candidate(candidate, candidate_hash);
//
//    return {candidate, candidate_hash};
//  }
//

//
//  std::pair<crypto::Hashed<runtime::PersistedValidationData,
//                           32,
//                           crypto::Blake2b_StreamHasher<32>>,
//            network::CommittedCandidateReceipt>
//  make_committed_candidate(ParachainId para_id,
//                           const Hash &relay_parent,
//                           BlockNumber relay_parent_number,
//                           const HeadData &parent_head,
//                           const HeadData &para_head,
//                           BlockNumber hrmp_watermark) {
//    crypto::Hashed<runtime::PersistedValidationData,
//                   32,
//                   crypto::Blake2b_StreamHasher<32>>
//        persisted_validation_data(runtime::PersistedValidationData{
//            .parent_head = parent_head,
//            .relay_parent_number = relay_parent_number,
//            .relay_parent_storage_root = hashFromStrData("69"),
//            .max_pov_size = 1000000,
//        });
//
//    network::CommittedCandidateReceipt candidate{
//        .descriptor =
//            network::CandidateDescriptor{
//                .para_id = para_id,
//                .relay_parent = relay_parent,
//                .collator_id = {},
//                .persisted_data_hash = persisted_validation_data.getHash(),
//                .pov_hash = hashFromStrData("1"),
//                .erasure_encoding_root = hashFromStrData("1"),
//                .signature = {},
//                .para_head_hash = hasher_->blake2b_256(para_head),
//                .validation_code_hash = hashFromStrData("42"),
//            },
//        .commitments =
//            network::CandidateCommitments{
//                .upward_msgs = {},
//                .outbound_hor_msgs = {},
//                .opt_para_runtime = std::nullopt,
//                .para_head = para_head,
//                .downward_msgs_count = 1,
//                .watermark = hrmp_watermark,
//            },
//    };
//
//    return std::make_pair(std::move(persisted_validation_data),
//                          std::move(candidate));
//  }
//
//  bool getNodePointerStorage(const fragment::NodePointer &p, size_t val) {
//    auto pa = kagome::if_type<const fragment::NodePointerStorage>(p);
//    return pa && pa->get() == val;
//  }
//
//  template <typename T>
//  bool compareVectors(const std::vector<T> &l, const std::vector<T> &r) {
//    return l == r;
//  }
//
//  bool compareMapsOfCandidates(const CandidatesHashMap &l,
//                               const CandidatesHashMap &r) {
//    return l == r;
//  }
//
//
//  Hash fromNumber(uint64_t n) const {
//    Hash h{};
//    *(uint64_t *)&h[0] = n;
//    return h;
//  }
//
//  void filterACByPara(TestState &test_state, ParachainId para_id) {
//    for (auto it = test_state.availability_cores.begin();
//         it != test_state.availability_cores.end();) {
//      const runtime::CoreState &cs = *it;
//      auto p = visit_in_place(
//          cs,
//          [](const runtime::OccupiedCore &core) mutable
//          -> std::optional<ParachainId> {
//            return core.candidate_descriptor.para_id;
//          },
//          [](const runtime::ScheduledCore &core) mutable
//          -> std::optional<ParachainId> { return core.para_id; },
//          [](runtime::FreeCore) -> std::optional<ParachainId> {
//            return std::nullopt;
//          });
//
//      if (p && *p == para_id) {
//        ++it;
//      } else {
//        it = test_state.availability_cores.erase(it);
//      }
//    }
//    ASSERT_EQ(test_state.availability_cores.size(), 1);
//  }
//
//

//
//
//  auto get_backable_candidates(
//      const TestLeaf &leaf,
//      ParachainId para_id,
//      std::vector<CandidateHash> required_path,
//      uint32_t count,
//      const std::vector<std::pair<CandidateHash, Hash>> &expected_result) {
//    auto resp = prospective_parachain_->answerGetBackableCandidates(
//        leaf.hash, para_id, count, required_path);
//    ASSERT_EQ(resp, expected_result);
//  }
//
//  auto get_hypothetical_frontier(
//      const CandidateHash &candidate_hash,
//      const network::CommittedCandidateReceipt &receipt,
//      const runtime::PersistedValidationData &persisted_validation_data,
//      const Hash &fragment_tree_relay_parent,
//      bool backed_in_path_only,
//      const std::vector<size_t> &expected_depths) {
//    HypotheticalCandidate hypothetical_candidate{HypotheticalCandidateComplete{
//        .candidate_hash = candidate_hash,
//        .receipt = receipt,
//        .persisted_validation_data = persisted_validation_data,
//    }};
//    auto resp = prospective_parachain_->answer_hypothetical_membership_request(
//        std::span<const HypotheticalCandidate>{&hypothetical_candidate, 1},
//        {{fragment_tree_relay_parent}},
//        backed_in_path_only);
//    std::vector<
//        std::pair<HypotheticalCandidate, fragment::FragmentTreeMembership>>
//        expected_frontier;
//    if (expected_depths.empty()) {
//      fragment::FragmentTreeMembership s{};
//      expected_frontier.emplace_back(hypothetical_candidate, s);
//    } else {
//      fragment::FragmentTreeMembership s{
//          {fragment_tree_relay_parent, expected_depths}};
//      expected_frontier.emplace_back(hypothetical_candidate, s);
//    };
//    ASSERT_EQ(resp.size(), expected_frontier.size());
//    for (size_t i = 0; i < resp.size(); ++i) {
//      const auto &[ll, lr] = resp[i];
//      const auto &[rl, rr] = expected_frontier[i];
//
//      ASSERT_TRUE(ll == rl);
//      ASSERT_EQ(lr, rr);
//    }
//  }
//
//  void back_candidate(const network::CommittedCandidateReceipt &candidate,
//                      const CandidateHash &candidate_hash) {
//    prospective_parachain_->candidate_backed(candidate.descriptor.para_id,
//                                            candidate_hash);
//  }
//
//  void second_candidate(const network::CommittedCandidateReceipt &candidate) {
//    prospective_parachain_->candidateSeconded(
//        candidate.descriptor.para_id,
//        network::candidateHash(*hasher_, candidate));
//  }
//
//  auto get_membership(ParachainId para_id,
//                      const CandidateHash &candidate_hash,
//                      const std::vector<std::pair<Hash, std::vector<size_t>>>
//                          &expected_membership_response) {
//    const auto resp = prospective_parachain_->answerTreeMembershipRequest(
//        para_id, candidate_hash);
//    ASSERT_EQ(resp, expected_membership_response);
//  }
//
//  void deactivate_leaf(const Hash &hash) {
//    network::ExView update{
//        .view = {},
//        .new_head = {},
//        .lost = {hash},
//    };
//    std::ignore =
//        prospective_parachain_->onActiveLeavesUpdate(network::ExViewRef{
//            .new_head = {},
//            .lost = update.lost,
//        });
//  }
//
//  auto get_pvd(
//      ParachainId para_id,
//      const Hash &candidate_relay_parent,
//      const HeadData &parent_head_data,
//      const std::optional<runtime::PersistedValidationData> &expected_pvd) {
//    auto resp = prospective_parachain_->answerProspectiveValidationDataRequest(
//        candidate_relay_parent,
//        hasher_->blake2b_256(parent_head_data),
//        para_id);
//    ASSERT_TRUE(resp.has_value());
//    ASSERT_EQ(resp.value(), expected_pvd);
//  }
//};

TEST_F(ProspectiveParachainsTest, should_do_no_work_if_async_backing_disabled_for_leaf) {
  network::ExView update{
      .view = {},
      .new_head =
          BlockHeader{
              .number = 1,
              .parent_hash = fromNumber(131),
              .state_root = {},
              .extrinsics_root = {},
              .digest = {},
              .hash_opt = {},
          },
      .lost = {},
  };
  const auto hash = fromNumber(130);
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
  TestState test_state(hasher_);
  const ParachainId chain_a{1};
  const ParachainId chain_b{2};

	std::map<CoreIndex, std::vector<ParachainId>> claim_queue;
	claim_queue[CoreIndex(0)] = [&]() -> std::vector<ParachainId> {
    return {chain_a, chain_b};
  }();
	test_state.claim_queue = std::move(claim_queue);

  TestLeaf leaf_a{
    .number = 100,
    .hash = fromNumber(130),
    .para_data = {
      {1, PerParaData(97, {1, 2, 3})},
      {2, PerParaData(100, {2, 3, 4})},
    },
  };

  TestLeaf leaf_b{
      .number = 101,
      .hash = fromNumber(131),
      .para_data =
          {
              {1, PerParaData(99, {3, 4, 5})},
              {2, PerParaData(101, {4, 5, 6})},
          },
  };
  TestLeaf leaf_c{
      .number = 102,
      .hash = fromNumber(132),
      .para_data =
          {
              {1, PerParaData(102, {5, 6, 7})},
              {2, PerParaData(98, {6, 7, 8})},
          },
  };

  fragment::AsyncBackingParams async_backing_params{
      .max_candidate_depth = 4,
      .allowed_ancestry_len = ALLOWED_ANCESTRY_LEN,
  };

  activate_leaf(leaf_a, test_state, async_backing_params);
  activate_leaf(leaf_b, test_state, async_backing_params);
  activate_leaf(leaf_c, test_state, async_backing_params);

  const auto &[candidate_a1, pvd_a1] =
      make_candidate(leaf_a.hash,
                     leaf_a.number,
                     1,
                     {1, 2, 3},
                     {1},
                     test_state.validation_code_hash);
  const Hash candidate_hash_a1 = hash(candidate_a1);
  std::vector<std::pair<Hash, Hash>> response_a1 = {{candidate_hash_a1, leaf_a.hash}};

  const auto &[candidate_a2, pvd_a2] =
      make_candidate(leaf_a.hash,
                     leaf_a.number,
                     2,
                     {2, 3, 4},
                     {2},
                     test_state.validation_code_hash);
  const Hash candidate_hash_a2 = hash(candidate_a2);
  std::vector<std::pair<Hash, Hash>> response_a2 = {{candidate_hash_a2, leaf_a.hash}};

  const auto &[candidate_b, pvd_b] =
      make_candidate(leaf_b.hash,
                     leaf_b.number,
                     1,
                     {3, 4, 5},
                     {3},
                     test_state.validation_code_hash);
  const Hash candidate_hash_b = hash(candidate_b);
  std::vector<std::pair<Hash, Hash>> response_b = {{candidate_hash_b, leaf_b.hash}};

  const auto &[candidate_c, pvd_c] =
      make_candidate(leaf_c.hash,
                     leaf_c.number,
                     2,
                     {6, 7, 8},
                     {4},
                     test_state.validation_code_hash);
  const Hash candidate_hash_c = hash(candidate_c);
  std::vector<std::pair<Hash, Hash>> response_c = {{candidate_hash_c, leaf_c.hash}};

		// Introduce candidates.
  introduce_seconded_candidate(candidate_a1, pvd_a1);
  introduce_seconded_candidate(candidate_a2, pvd_a2);
  introduce_seconded_candidate(candidate_b, pvd_b);
  introduce_seconded_candidate(candidate_c, pvd_c);

		// Back candidates. Otherwise, we cannot check membership with GetBackableCandidates.
		//back_candidate(candidate_a1, candidate_hash_a1);
		//back_candidate(candidate_a2, candidate_hash_a2);
		//back_candidate(candidate_b, candidate_hash_b);
		//back_candidate(candidate_c, candidate_hash_c);

  ASSERT_EQ(prospective_parachain_->view().active_leaves.size(), 3);
}
