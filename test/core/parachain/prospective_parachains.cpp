/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"
#include "testutil/prepare_loggers.hpp"

#include "crypto/hasher/hasher_impl.hpp"
#include "crypto/type_hasher.hpp"
#include "parachain/types.hpp"
#include "parachain/validator/fragment_tree.hpp"
#include "parachain/validator/impl/candidates.hpp"
#include "parachain/validator/prospective_parachains.hpp"
#include "runtime/runtime_api/parachain_host_types.hpp"
#include "scale/kagome_scale.hpp"
#include "scale/scale.hpp"
#include "testutil/scale_test_comparator.hpp"

using namespace kagome::primitives;
using namespace kagome::parachain;
namespace network = kagome::network;
namespace runtime = kagome::runtime;
namespace common = kagome::common;
namespace crypto = kagome::crypto;

struct PerParaData {
  BlockNumber min_relay_parent;
  HeadData head_data;
  std::vector<fragment::CandidatePendingAvailability> pending_availability;
};

struct TestState {
  std::vector<runtime::CoreState> availability_cores;
  ValidationCodeHash validation_code_hash;

  TestState()
      : availability_cores{{runtime::ScheduledCore{.para_id = ParachainId{1},
                                                   .collator = std::nullopt},
                            runtime::ScheduledCore{.para_id = ParachainId{2},
                                                   .collator = std::nullopt}}},
        validation_code_hash{Hash::fromString("42").value()} {}
};

class ProspectiveParachainsTest : public testing::Test {
  void SetUp() override {
    testutil::prepareLoggers();
    hasher_ = std::make_shared<kagome::crypto::HasherImpl>();
  }

 protected:
  using CandidatesHashMap = std::unordered_map<
      Hash,
      std::unordered_map<ParachainId, std::unordered_set<CandidateHash>>>;

  std::shared_ptr<kagome::crypto::Hasher> hasher_;

  Hash hashFromStrData(std::span<const char> data) {
    return hasher_->blake2b_256(data);
  }

  fragment::Constraints make_constraints(
      BlockNumber min_relay_parent_number,
      std::vector<BlockNumber> valid_watermarks,
      HeadData required_parent) {
    return fragment::Constraints{
        .min_relay_parent_number = min_relay_parent_number,
        .max_pov_size = 1000000,
        .max_code_size = 1000000,
        .ump_remaining = 10,
        .ump_remaining_bytes = 1000,
        .max_ump_num_per_candidate = 10,
        .dmp_remaining_messages = std::vector<BlockNumber>(10, 0),
        .hrmp_inbound = fragment::InboundHrmpLimitations{.valid_watermarks =
                                                             valid_watermarks},
        .hrmp_channels_out = {},
        .max_hrmp_num_per_candidate = 0,
        .required_parent = required_parent,
        .validation_code_hash = hashFromStrData("42"),
        .upgrade_restriction = std::nullopt,
        .future_validation_code = std::nullopt,
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

  std::pair<crypto::Hashed<runtime::PersistedValidationData,
                           32,
                           crypto::Blake2b_StreamHasher<32>>,
            network::CommittedCandidateReceipt>
  make_committed_candidate(ParachainId para_id,
                           const Hash &relay_parent,
                           BlockNumber relay_parent_number,
                           const HeadData &parent_head,
                           const HeadData &para_head,
                           BlockNumber hrmp_watermark) {
    crypto::Hashed<runtime::PersistedValidationData,
                   32,
                   crypto::Blake2b_StreamHasher<32>>
        persisted_validation_data(runtime::PersistedValidationData{
            .parent_head = parent_head,
            .relay_parent_number = relay_parent_number,
            .relay_parent_storage_root = hashFromStrData("69"),
            .max_pov_size = 1000000,
        });

    network::CommittedCandidateReceipt candidate{
        .descriptor =
            network::CandidateDescriptor{
                .para_id = para_id,
                .relay_parent = relay_parent,
                .collator_id = {},
                .persisted_data_hash = persisted_validation_data.getHash(),
                .pov_hash = hashFromStrData("1"),
                .erasure_encoding_root = hashFromStrData("1"),
                .signature = {},
                .para_head_hash = hasher_->blake2b_256(para_head),
                .validation_code_hash = hashFromStrData("42"),
            },
        .commitments =
            network::CandidateCommitments{
                .upward_msgs = {},
                .outbound_hor_msgs = {},
                .opt_para_runtime = std::nullopt,
                .para_head = para_head,
                .downward_msgs_count = 1,
                .watermark = hrmp_watermark,
            },
    };

    return std::make_pair(std::move(persisted_validation_data),
                          std::move(candidate));
  }

  bool getNodePointerStorage(const fragment::NodePointer &p, size_t val) {
    auto pa = kagome::if_type<const fragment::NodePointerStorage>(p);
    return pa && pa->get() == val;
  }

  template <typename T>
  bool compareVectors(const std::vector<T> &l, const std::vector<T> &r) {
    return l == r;
  }

  bool compareMapsOfCandidates(const CandidatesHashMap &l,
                               const CandidatesHashMap &r) {
    return l == r;
  }
};

/*TEST_F(ProspectiveParachainsTest, sendCandidatesAndCheckIfFound) {
  TestState test_state{};
  network::ExView{
      .view = {},
      .new_head =
          BlockHeader{
              .number = 100,
              .parent_hash = hasher_->blake2b_256(
                  testutil::scaleEncodeAndCompareWithRef(BlockNumber{99})
                      .value()),
              .state_root = {},
              .extrinsics_root = {},
              .digest = {},
              .hash_opt = {},
          },
      .lost = {},
  };
}*/

TEST_F(ProspectiveParachainsTest,
       FragmentTree_scopeRejectsAncestorsThatSkipBlocks) {
  ParachainId para_id{5};
  fragment::RelayChainBlockInfo relay_parent{
      .hash = hashFromStrData("10"),
      .number = 10,
      .storage_root = hashFromStrData("69"),
  };

  std::vector<fragment::RelayChainBlockInfo> ancestors = {
      fragment::RelayChainBlockInfo{
          .hash = hashFromStrData("8"),
          .number = 8,
          .storage_root = hashFromStrData("69"),
      }};

  const size_t max_depth = 2ull;
  fragment::Constraints base_constraints(
      make_constraints(8, {8, 9}, {1, 2, 3}));
  ASSERT_EQ(
      fragment::Scope::withAncestors(
          para_id, relay_parent, base_constraints, {}, max_depth, ancestors)
          .error(),
      fragment::Scope::Error::UNEXPECTED_ANCESTOR);
}

TEST_F(ProspectiveParachainsTest,
       FragmentTree_scopeRejectsAncestorFor_0_Block) {
  ParachainId para_id{5};
  fragment::RelayChainBlockInfo relay_parent{
      .hash = hashFromStrData("0"),
      .number = 0,
      .storage_root = hashFromStrData("69"),
  };

  std::vector<fragment::RelayChainBlockInfo> ancestors = {
      fragment::RelayChainBlockInfo{
          .hash = hashFromStrData("99"),
          .number = 99999,
          .storage_root = hashFromStrData("69"),
      }};

  const size_t max_depth = 2ull;
  fragment::Constraints base_constraints(make_constraints(0, {}, {1, 2, 3}));
  ASSERT_EQ(
      fragment::Scope::withAncestors(
          para_id, relay_parent, base_constraints, {}, max_depth, ancestors)
          .error(),
      fragment::Scope::Error::UNEXPECTED_ANCESTOR);
}

TEST_F(ProspectiveParachainsTest, FragmentTree_scopeOnlyTakesAncestorsUpToMin) {
  ParachainId para_id{5};
  fragment::RelayChainBlockInfo relay_parent{
      .hash = hashFromStrData("0"),
      .number = 5,
      .storage_root = hashFromStrData("69"),
  };

  std::vector<fragment::RelayChainBlockInfo> ancestors = {
      fragment::RelayChainBlockInfo{
          .hash = hashFromStrData("4"),
          .number = 4,
          .storage_root = hashFromStrData("69"),
      },
      fragment::RelayChainBlockInfo{
          .hash = hashFromStrData("3"),
          .number = 3,
          .storage_root = hashFromStrData("69"),
      },
      fragment::RelayChainBlockInfo{
          .hash = hashFromStrData("2"),
          .number = 2,
          .storage_root = hashFromStrData("69"),
      }};

  const size_t max_depth = 2ull;
  fragment::Constraints base_constraints(make_constraints(3, {2}, {1, 2, 3}));
  auto scope =
      fragment::Scope::withAncestors(
          para_id, relay_parent, base_constraints, {}, max_depth, ancestors)
          .value();

  ASSERT_EQ(scope.ancestors.size(), 2);
  ASSERT_EQ(scope.ancestors_by_hash.size(), 2);
}

TEST_F(ProspectiveParachainsTest, Storage_AddCandidate) {
  fragment::CandidateStorage storage{};
  Hash relay_parent(hashFromStrData("69"));

  const auto &[pvd, candidate] =
      make_committed_candidate(5, relay_parent, 8, {4, 5, 6}, {1, 2, 3}, 7);

  const Hash candidate_hash = network::candidateHash(*hasher_, candidate);
  const Hash parent_head_hash = hasher_->blake2b_256(pvd.get().parent_head);

  ASSERT_TRUE(
      storage.addCandidate(candidate_hash, candidate, pvd.get(), hasher_)
          .has_value());
  ASSERT_TRUE(storage.contains(candidate_hash));

  size_t counter = 0ull;
  storage.iterParaChildren(parent_head_hash, [&](const auto &) { ++counter; });
  ASSERT_EQ(1, counter);

  auto h = storage.relayParentByCandidateHash(candidate_hash);
  ASSERT_TRUE(h);
  ASSERT_EQ(*h, relay_parent);
}

TEST_F(ProspectiveParachainsTest, Storage_PopulateWorksRecursively) {
  fragment::CandidateStorage storage{};
  ParachainId para_id{5};

  Hash relay_parent_a(hashFromStrData("1"));
  Hash relay_parent_b(hashFromStrData("2"));

  const auto &[pvd_a, candidate_a] =
      make_committed_candidate(para_id, relay_parent_a, 0, {0x0a}, {0x0b}, 0);
  const Hash candidate_a_hash = network::candidateHash(*hasher_, candidate_a);

  const auto &[pvd_b, candidate_b] =
      make_committed_candidate(para_id, relay_parent_b, 1, {0x0b}, {0x0c}, 1);
  const Hash candidate_b_hash = network::candidateHash(*hasher_, candidate_b);

  fragment::Constraints base_constraints(make_constraints(0, {0}, {0x0a}));
  std::vector<fragment::RelayChainBlockInfo> ancestors = {
      fragment::RelayChainBlockInfo{
          .hash = relay_parent_a,
          .number = pvd_a.get().relay_parent_number,
          .storage_root = pvd_a.get().relay_parent_storage_root,
      }};

  fragment::RelayChainBlockInfo relay_parent_b_info{
      .hash = relay_parent_b,
      .number = pvd_b.get().relay_parent_number,
      .storage_root = pvd_b.get().relay_parent_storage_root,
  };

  ASSERT_TRUE(
      storage.addCandidate(candidate_a_hash, candidate_a, pvd_a.get(), hasher_)
          .has_value());

  ASSERT_TRUE(
      storage.addCandidate(candidate_b_hash, candidate_b, pvd_b.get(), hasher_)
          .has_value());

  auto scope =
      fragment::Scope::withAncestors(
          para_id, relay_parent_b_info, base_constraints, {}, 4ull, ancestors)
          .value();

  fragment::FragmentTree tree =
      fragment::FragmentTree::populate(hasher_, scope, storage);
  std::vector<CandidateHash> candidates = tree.getCandidates();

  ASSERT_EQ(candidates.size(), 2);

  ASSERT_TRUE(std::find(candidates.begin(), candidates.end(), candidate_a_hash)
              != candidates.end());
  ASSERT_TRUE(std::find(candidates.begin(), candidates.end(), candidate_b_hash)
              != candidates.end());

  ASSERT_EQ(tree.nodes.size(), 2);
  ASSERT_TRUE(kagome::is_type<fragment::NodePointerRoot>(tree.nodes[0].parent));
  ASSERT_EQ(tree.nodes[0].candidate_hash, candidate_a_hash);
  ASSERT_EQ(tree.nodes[0].depth, 0);

  auto pa = kagome::if_type<fragment::NodePointerStorage>(tree.nodes[1].parent);
  ASSERT_TRUE(pa && pa->get() == 0);
  ASSERT_EQ(tree.nodes[1].candidate_hash, candidate_b_hash);
  ASSERT_EQ(tree.nodes[1].depth, 1);
}

TEST_F(ProspectiveParachainsTest, Storage_childrenOfRootAreContiguous) {
  fragment::CandidateStorage storage{};
  ParachainId para_id{5};

  Hash relay_parent_a(hashFromStrData("1"));
  Hash relay_parent_b(hashFromStrData("2"));

  const auto &[pvd_a, candidate_a] =
      make_committed_candidate(para_id, relay_parent_a, 0, {0x0a}, {0x0b}, 0);
  const Hash candidate_a_hash = network::candidateHash(*hasher_, candidate_a);

  const auto &[pvd_b, candidate_b] =
      make_committed_candidate(para_id, relay_parent_b, 1, {0x0b}, {0x0c}, 1);
  const Hash candidate_b_hash = network::candidateHash(*hasher_, candidate_b);

  const auto &[pvd_a2, candidate_a2] = make_committed_candidate(
      para_id, relay_parent_a, 0, {0x0a}, {0x0b, 1}, 0);
  const Hash candidate_a2_hash = network::candidateHash(*hasher_, candidate_a2);

  fragment::Constraints base_constraints(make_constraints(0, {0}, {0x0a}));
  std::vector<fragment::RelayChainBlockInfo> ancestors = {
      fragment::RelayChainBlockInfo{
          .hash = relay_parent_a,
          .number = pvd_a.get().relay_parent_number,
          .storage_root = pvd_a.get().relay_parent_storage_root,
      }};

  fragment::RelayChainBlockInfo relay_parent_b_info{
      .hash = relay_parent_b,
      .number = pvd_b.get().relay_parent_number,
      .storage_root = pvd_b.get().relay_parent_storage_root,
  };

  ASSERT_TRUE(
      storage.addCandidate(candidate_a_hash, candidate_a, pvd_a.get(), hasher_)
          .has_value());

  ASSERT_TRUE(
      storage.addCandidate(candidate_b_hash, candidate_b, pvd_b.get(), hasher_)
          .has_value());

  auto scope =
      fragment::Scope::withAncestors(
          para_id, relay_parent_b_info, base_constraints, {}, 4ull, ancestors)
          .value();

  fragment::FragmentTree tree =
      fragment::FragmentTree::populate(hasher_, scope, storage);
  ASSERT_TRUE(
      storage
          .addCandidate(candidate_a2_hash, candidate_a2, pvd_a2.get(), hasher_)
          .has_value());

  tree.addAndPopulate(candidate_a2_hash, storage);
  std::vector<CandidateHash> candidates = tree.getCandidates();

  ASSERT_EQ(candidates.size(), 3);
  ASSERT_TRUE(kagome::is_type<fragment::NodePointerRoot>(tree.nodes[0].parent));
  ASSERT_TRUE(kagome::is_type<fragment::NodePointerRoot>(tree.nodes[1].parent));

  auto pa = kagome::if_type<fragment::NodePointerStorage>(tree.nodes[2].parent);
  ASSERT_TRUE(pa && pa->get() == 0);
}

TEST_F(ProspectiveParachainsTest, Storage_addCandidateChildOfRoot) {
  fragment::CandidateStorage storage{};
  ParachainId para_id{5};
  Hash relay_parent_a(hashFromStrData("1"));

  const auto &[pvd_a, candidate_a] =
      make_committed_candidate(para_id, relay_parent_a, 0, {0x0a}, {0x0b}, 0);
  const Hash candidate_a_hash = network::candidateHash(*hasher_, candidate_a);

  const auto &[pvd_b, candidate_b] =
      make_committed_candidate(para_id, relay_parent_a, 0, {0x0a}, {0x0c}, 0);
  const Hash candidate_b_hash = network::candidateHash(*hasher_, candidate_b);

  fragment::Constraints base_constraints(make_constraints(0, {0}, {0x0a}));
  fragment::RelayChainBlockInfo relay_parent_a_info{
      .hash = relay_parent_a,
      .number = pvd_a.get().relay_parent_number,
      .storage_root = pvd_a.get().relay_parent_storage_root,
  };

  ASSERT_TRUE(
      storage.addCandidate(candidate_a_hash, candidate_a, pvd_a.get(), hasher_)
          .has_value());

  auto scope = fragment::Scope::withAncestors(
                   para_id, relay_parent_a_info, base_constraints, {}, 4ull, {})
                   .value();

  fragment::FragmentTree tree =
      fragment::FragmentTree::populate(hasher_, scope, storage);
  ASSERT_TRUE(
      storage.addCandidate(candidate_b_hash, candidate_b, pvd_b.get(), hasher_)
          .has_value());

  tree.addAndPopulate(candidate_b_hash, storage);
  std::vector<CandidateHash> candidates = tree.getCandidates();

  ASSERT_EQ(candidates.size(), 2);
  ASSERT_TRUE(kagome::is_type<fragment::NodePointerRoot>(tree.nodes[0].parent));
  ASSERT_TRUE(kagome::is_type<fragment::NodePointerRoot>(tree.nodes[1].parent));
}

TEST_F(ProspectiveParachainsTest, Storage_addCandidateChildOfNonRoot) {
  fragment::CandidateStorage storage{};
  ParachainId para_id{5};
  Hash relay_parent_a(hashFromStrData("1"));

  const auto &[pvd_a, candidate_a] =
      make_committed_candidate(para_id, relay_parent_a, 0, {0x0a}, {0x0b}, 0);
  const Hash candidate_a_hash = network::candidateHash(*hasher_, candidate_a);

  const auto &[pvd_b, candidate_b] =
      make_committed_candidate(para_id, relay_parent_a, 0, {0x0b}, {0x0c}, 0);
  const Hash candidate_b_hash = network::candidateHash(*hasher_, candidate_b);

  fragment::Constraints base_constraints(make_constraints(0, {0}, {0x0a}));
  fragment::RelayChainBlockInfo relay_parent_a_info{
      .hash = relay_parent_a,
      .number = pvd_a.get().relay_parent_number,
      .storage_root = pvd_a.get().relay_parent_storage_root,
  };

  ASSERT_TRUE(
      storage.addCandidate(candidate_a_hash, candidate_a, pvd_a.get(), hasher_)
          .has_value());

  auto scope = fragment::Scope::withAncestors(
                   para_id, relay_parent_a_info, base_constraints, {}, 4ull, {})
                   .value();

  fragment::FragmentTree tree =
      fragment::FragmentTree::populate(hasher_, scope, storage);
  ASSERT_TRUE(
      storage.addCandidate(candidate_b_hash, candidate_b, pvd_b.get(), hasher_)
          .has_value());

  tree.addAndPopulate(candidate_b_hash, storage);
  std::vector<CandidateHash> candidates = tree.getCandidates();

  ASSERT_EQ(candidates.size(), 2);
  ASSERT_TRUE(kagome::is_type<fragment::NodePointerRoot>(tree.nodes[0].parent));
  auto pa = kagome::if_type<fragment::NodePointerStorage>(tree.nodes[1].parent);
  ASSERT_TRUE(pa && pa->get() == 0);
}

TEST_F(ProspectiveParachainsTest, Storage_gracefulCycleOf_0) {
  fragment::CandidateStorage storage{};
  ParachainId para_id{5};
  Hash relay_parent_a(hashFromStrData("1"));

  const auto &[pvd_a, candidate_a] =
      make_committed_candidate(para_id, relay_parent_a, 0, {0x0a}, {0x0a}, 0);
  const Hash candidate_a_hash = network::candidateHash(*hasher_, candidate_a);

  fragment::Constraints base_constraints(make_constraints(0, {0}, {0x0a}));
  fragment::RelayChainBlockInfo relay_parent_a_info{
      .hash = relay_parent_a,
      .number = pvd_a.get().relay_parent_number,
      .storage_root = pvd_a.get().relay_parent_storage_root,
  };

  const size_t max_depth = 4ull;
  ASSERT_TRUE(
      storage.addCandidate(candidate_a_hash, candidate_a, pvd_a.get(), hasher_)
          .has_value());
  auto scope =
      fragment::Scope::withAncestors(
          para_id, relay_parent_a_info, base_constraints, {}, max_depth, {})
          .value();

  fragment::FragmentTree tree =
      fragment::FragmentTree::populate(hasher_, scope, storage);
  std::vector<CandidateHash> candidates = tree.getCandidates();

  ASSERT_EQ(candidates.size(), 1);
  ASSERT_EQ(tree.nodes.size(), max_depth + 1);

  ASSERT_TRUE(kagome::is_type<fragment::NodePointerRoot>(tree.nodes[0].parent));
  ASSERT_TRUE(getNodePointerStorage(tree.nodes[1].parent, 0));
  ASSERT_TRUE(getNodePointerStorage(tree.nodes[2].parent, 1));
  ASSERT_TRUE(getNodePointerStorage(tree.nodes[3].parent, 2));
  ASSERT_TRUE(getNodePointerStorage(tree.nodes[4].parent, 3));

  ASSERT_EQ(tree.nodes[0].candidate_hash, candidate_a_hash);
  ASSERT_EQ(tree.nodes[1].candidate_hash, candidate_a_hash);
  ASSERT_EQ(tree.nodes[2].candidate_hash, candidate_a_hash);
  ASSERT_EQ(tree.nodes[3].candidate_hash, candidate_a_hash);
  ASSERT_EQ(tree.nodes[4].candidate_hash, candidate_a_hash);
}

TEST_F(ProspectiveParachainsTest, Storage_gracefulCycleOf_1) {
  fragment::CandidateStorage storage{};
  ParachainId para_id{5};
  Hash relay_parent_a(hashFromStrData("1"));

  const auto &[pvd_a, candidate_a] =
      make_committed_candidate(para_id, relay_parent_a, 0, {0x0a}, {0x0b}, 0);
  const Hash candidate_a_hash = network::candidateHash(*hasher_, candidate_a);

  const auto &[pvd_b, candidate_b] =
      make_committed_candidate(para_id, relay_parent_a, 0, {0x0b}, {0x0a}, 0);
  const Hash candidate_b_hash = network::candidateHash(*hasher_, candidate_b);

  fragment::Constraints base_constraints(make_constraints(0, {0}, {0x0a}));
  fragment::RelayChainBlockInfo relay_parent_a_info{
      .hash = relay_parent_a,
      .number = pvd_a.get().relay_parent_number,
      .storage_root = pvd_a.get().relay_parent_storage_root,
  };

  const size_t max_depth = 4ull;
  ASSERT_TRUE(
      storage.addCandidate(candidate_a_hash, candidate_a, pvd_a.get(), hasher_)
          .has_value());
  ASSERT_TRUE(
      storage.addCandidate(candidate_b_hash, candidate_b, pvd_b.get(), hasher_)
          .has_value());
  auto scope =
      fragment::Scope::withAncestors(
          para_id, relay_parent_a_info, base_constraints, {}, max_depth, {})
          .value();

  fragment::FragmentTree tree =
      fragment::FragmentTree::populate(hasher_, scope, storage);
  std::vector<CandidateHash> candidates = tree.getCandidates();

  ASSERT_EQ(candidates.size(), 2);
  ASSERT_EQ(tree.nodes.size(), max_depth + 1);

  ASSERT_TRUE(kagome::is_type<fragment::NodePointerRoot>(tree.nodes[0].parent));
  ASSERT_TRUE(getNodePointerStorage(tree.nodes[1].parent, 0));
  ASSERT_TRUE(getNodePointerStorage(tree.nodes[2].parent, 1));
  ASSERT_TRUE(getNodePointerStorage(tree.nodes[3].parent, 2));
  ASSERT_TRUE(getNodePointerStorage(tree.nodes[4].parent, 3));

  ASSERT_EQ(tree.nodes[0].candidate_hash, candidate_a_hash);
  ASSERT_EQ(tree.nodes[1].candidate_hash, candidate_b_hash);
  ASSERT_EQ(tree.nodes[2].candidate_hash, candidate_a_hash);
  ASSERT_EQ(tree.nodes[3].candidate_hash, candidate_b_hash);
  ASSERT_EQ(tree.nodes[4].candidate_hash, candidate_a_hash);
}

TEST_F(ProspectiveParachainsTest, Storage_hypotheticalDepthsKnownAndUnknown) {
  fragment::CandidateStorage storage{};
  ParachainId para_id{5};
  Hash relay_parent_a(hashFromStrData("1"));

  const auto &[pvd_a, candidate_a] =
      make_committed_candidate(para_id, relay_parent_a, 0, {0x0a}, {0x0b}, 0);
  const Hash candidate_a_hash = network::candidateHash(*hasher_, candidate_a);

  const auto &[pvd_b, candidate_b] =
      make_committed_candidate(para_id, relay_parent_a, 0, {0x0b}, {0x0a}, 0);
  const Hash candidate_b_hash = network::candidateHash(*hasher_, candidate_b);

  fragment::Constraints base_constraints(make_constraints(0, {0}, {0x0a}));
  fragment::RelayChainBlockInfo relay_parent_a_info{
      .hash = relay_parent_a,
      .number = pvd_a.get().relay_parent_number,
      .storage_root = pvd_a.get().relay_parent_storage_root,
  };

  const size_t max_depth = 4ull;
  ASSERT_TRUE(
      storage.addCandidate(candidate_a_hash, candidate_a, pvd_a.get(), hasher_)
          .has_value());
  ASSERT_TRUE(
      storage.addCandidate(candidate_b_hash, candidate_b, pvd_b.get(), hasher_)
          .has_value());
  auto scope =
      fragment::Scope::withAncestors(
          para_id, relay_parent_a_info, base_constraints, {}, max_depth, {})
          .value();

  fragment::FragmentTree tree =
      fragment::FragmentTree::populate(hasher_, scope, storage);
  std::vector<CandidateHash> candidates = tree.getCandidates();

  ASSERT_EQ(candidates.size(), 2);
  ASSERT_EQ(tree.nodes.size(), max_depth + 1);

  ASSERT_TRUE(compareVectors(
      tree.hypotheticalDepths(candidate_a_hash,
                              HypotheticalCandidateIncomplete{
                                  .candidate_hash = {},
                                  .candidate_para = 0,
                                  .parent_head_data_hash = hasher_->blake2b_256(
                                      std::vector<uint8_t>{0x0a}),
                                  .candidate_relay_parent = relay_parent_a,
                              },
                              storage,
                              false),
      {0, 2, 4}));
  ASSERT_TRUE(compareVectors(
      tree.hypotheticalDepths(candidate_b_hash,
                              HypotheticalCandidateIncomplete{
                                  .candidate_hash = {},
                                  .candidate_para = 0,
                                  .parent_head_data_hash = hasher_->blake2b_256(
                                      std::vector<uint8_t>{0x0b}),
                                  .candidate_relay_parent = relay_parent_a,
                              },
                              storage,
                              false),
      {1, 3}));
  ASSERT_TRUE(compareVectors(
      tree.hypotheticalDepths(hashFromStrData("21"),
                              HypotheticalCandidateIncomplete{
                                  .candidate_hash = {},
                                  .candidate_para = 0,
                                  .parent_head_data_hash = hasher_->blake2b_256(
                                      std::vector<uint8_t>{0x0a}),
                                  .candidate_relay_parent = relay_parent_a,
                              },
                              storage,
                              false),
      {0, 2, 4}));
  ASSERT_TRUE(compareVectors(
      tree.hypotheticalDepths(hashFromStrData("22"),
                              HypotheticalCandidateIncomplete{
                                  .candidate_hash = {},
                                  .candidate_para = 0,
                                  .parent_head_data_hash = hasher_->blake2b_256(
                                      std::vector<uint8_t>{0x0b}),
                                  .candidate_relay_parent = relay_parent_a,
                              },
                              storage,
                              false),
      {1, 3}));
}

TEST_F(ProspectiveParachainsTest,
       Storage_hypotheticalDepthsStricterOnComplete) {
  fragment::CandidateStorage storage{};
  ParachainId para_id{5};
  Hash relay_parent_a(hashFromStrData("1"));

  const auto &[pvd_a, candidate_a] = make_committed_candidate(
      para_id, relay_parent_a, 0, {0x0a}, {0x0b}, 1000);
  const Hash candidate_a_hash = network::candidateHash(*hasher_, candidate_a);

  fragment::Constraints base_constraints(make_constraints(0, {0}, {0x0a}));
  fragment::RelayChainBlockInfo relay_parent_a_info{
      .hash = relay_parent_a,
      .number = pvd_a.get().relay_parent_number,
      .storage_root = pvd_a.get().relay_parent_storage_root,
  };

  const size_t max_depth = 4ull;
  auto scope =
      fragment::Scope::withAncestors(
          para_id, relay_parent_a_info, base_constraints, {}, max_depth, {})
          .value();

  fragment::FragmentTree tree =
      fragment::FragmentTree::populate(hasher_, scope, storage);

  ASSERT_TRUE(compareVectors(
      tree.hypotheticalDepths(candidate_a_hash,
                              HypotheticalCandidateIncomplete{
                                  .candidate_hash = {},
                                  .candidate_para = 0,
                                  .parent_head_data_hash = hasher_->blake2b_256(
                                      std::vector<uint8_t>{0x0a}),
                                  .candidate_relay_parent = relay_parent_a,
                              },
                              storage,
                              false),
      {0}));
  ASSERT_TRUE(
      tree.hypotheticalDepths(candidate_a_hash,
                              HypotheticalCandidateComplete{
                                  .candidate_hash = {},
                                  .receipt = candidate_a,
                                  .persisted_validation_data = pvd_a.get(),
                              },
                              storage,
                              false)
          .empty());
}

TEST_F(ProspectiveParachainsTest, Storage_hypotheticalDepthsBackedInPath) {
  fragment::CandidateStorage storage{};
  ParachainId para_id{5};
  Hash relay_parent_a(hashFromStrData("1"));

  const auto &[pvd_a, candidate_a] =
      make_committed_candidate(para_id, relay_parent_a, 0, {0x0a}, {0x0b}, 0);
  const Hash candidate_a_hash = network::candidateHash(*hasher_, candidate_a);

  const auto &[pvd_b, candidate_b] =
      make_committed_candidate(para_id, relay_parent_a, 0, {0x0b}, {0x0c}, 0);
  const Hash candidate_b_hash = network::candidateHash(*hasher_, candidate_b);

  const auto &[pvd_c, candidate_c] =
      make_committed_candidate(para_id, relay_parent_a, 0, {0x0b}, {0x0d}, 0);
  const Hash candidate_c_hash = network::candidateHash(*hasher_, candidate_c);

  fragment::Constraints base_constraints(make_constraints(0, {0}, {0x0a}));
  fragment::RelayChainBlockInfo relay_parent_a_info{
      .hash = relay_parent_a,
      .number = pvd_a.get().relay_parent_number,
      .storage_root = pvd_a.get().relay_parent_storage_root,
  };

  const size_t max_depth = 4ull;
  ASSERT_TRUE(
      storage.addCandidate(candidate_a_hash, candidate_a, pvd_a.get(), hasher_)
          .has_value());
  ASSERT_TRUE(
      storage.addCandidate(candidate_b_hash, candidate_b, pvd_b.get(), hasher_)
          .has_value());
  ASSERT_TRUE(
      storage.addCandidate(candidate_c_hash, candidate_c, pvd_c.get(), hasher_)
          .has_value());

  storage.markBacked(candidate_a_hash);
  storage.markBacked(candidate_b_hash);

  auto scope =
      fragment::Scope::withAncestors(
          para_id, relay_parent_a_info, base_constraints, {}, max_depth, {})
          .value();

  fragment::FragmentTree tree =
      fragment::FragmentTree::populate(hasher_, scope, storage);
  std::vector<CandidateHash> candidates = tree.getCandidates();

  ASSERT_EQ(candidates.size(), 3);
  ASSERT_EQ(tree.nodes.size(), 3);

  Hash candidate_d_hash(hashFromStrData("AA"));
  ASSERT_TRUE(compareVectors(
      tree.hypotheticalDepths(candidate_d_hash,
                              HypotheticalCandidateIncomplete{
                                  .candidate_hash = {},
                                  .candidate_para = 0,
                                  .parent_head_data_hash = hasher_->blake2b_256(
                                      std::vector<uint8_t>{0x0a}),
                                  .candidate_relay_parent = relay_parent_a,
                              },
                              storage,
                              true),
      {0}));
  ASSERT_TRUE(compareVectors(
      tree.hypotheticalDepths(candidate_d_hash,
                              HypotheticalCandidateIncomplete{
                                  .candidate_hash = {},
                                  .candidate_para = 0,
                                  .parent_head_data_hash = hasher_->blake2b_256(
                                      std::vector<uint8_t>{0x0c}),
                                  .candidate_relay_parent = relay_parent_a,
                              },
                              storage,
                              true),
      {2}));
  ASSERT_TRUE(compareVectors(
      tree.hypotheticalDepths(candidate_d_hash,
                              HypotheticalCandidateIncomplete{
                                  .candidate_hash = {},
                                  .candidate_para = 0,
                                  .parent_head_data_hash = hasher_->blake2b_256(
                                      std::vector<uint8_t>{0x0d}),
                                  .candidate_relay_parent = relay_parent_a,
                              },
                              storage,
                              true),
      {}));
  ASSERT_TRUE(compareVectors(
      tree.hypotheticalDepths(candidate_d_hash,
                              HypotheticalCandidateIncomplete{
                                  .candidate_hash = {},
                                  .candidate_para = 0,
                                  .parent_head_data_hash = hasher_->blake2b_256(
                                      std::vector<uint8_t>{0x0d}),
                                  .candidate_relay_parent = relay_parent_a,
                              },
                              storage,
                              false),
      {2}));
}

TEST_F(ProspectiveParachainsTest, Storage_pendingAvailabilityInScope) {
  fragment::CandidateStorage storage{};
  ParachainId para_id{5};
  Hash relay_parent_a(hashFromStrData("1"));
  Hash relay_parent_b(hashFromStrData("2"));
  Hash relay_parent_c(hashFromStrData("3"));

  const auto &[pvd_a, candidate_a] =
      make_committed_candidate(para_id, relay_parent_a, 0, {0x0a}, {0x0b}, 0);
  const Hash candidate_a_hash = network::candidateHash(*hasher_, candidate_a);

  const auto &[pvd_b, candidate_b] =
      make_committed_candidate(para_id, relay_parent_b, 1, {0x0b}, {0x0c}, 1);
  const Hash candidate_b_hash = network::candidateHash(*hasher_, candidate_b);

  fragment::Constraints base_constraints(make_constraints(1, {}, {0x0a}));
  fragment::RelayChainBlockInfo relay_parent_a_info{
      .hash = relay_parent_a,
      .number = pvd_a.get().relay_parent_number,
      .storage_root = pvd_a.get().relay_parent_storage_root,
  };
  std::vector<fragment::PendingAvailability> pending_availability = {
      fragment::PendingAvailability{
          .candidate_hash = candidate_a_hash,
          .relay_parent = relay_parent_a_info,
      }};
  fragment::RelayChainBlockInfo relay_parent_b_info{
      .hash = relay_parent_b,
      .number = pvd_b.get().relay_parent_number,
      .storage_root = pvd_b.get().relay_parent_storage_root,
  };
  fragment::RelayChainBlockInfo relay_parent_c_info{
      .hash = relay_parent_c,
      .number = pvd_b.get().relay_parent_number + 1,
      .storage_root = {},
  };

  const size_t max_depth = 4ull;
  ASSERT_TRUE(
      storage.addCandidate(candidate_a_hash, candidate_a, pvd_a.get(), hasher_)
          .has_value());
  ASSERT_TRUE(
      storage.addCandidate(candidate_b_hash, candidate_b, pvd_b.get(), hasher_)
          .has_value());

  storage.markBacked(candidate_a_hash);
  auto scope = fragment::Scope::withAncestors(para_id,
                                              relay_parent_c_info,
                                              base_constraints,
                                              pending_availability,
                                              max_depth,
                                              {relay_parent_b_info})
                   .value();

  fragment::FragmentTree tree =
      fragment::FragmentTree::populate(hasher_, scope, storage);
  std::vector<CandidateHash> candidates = tree.getCandidates();

  ASSERT_EQ(candidates.size(), 2);
  ASSERT_EQ(tree.nodes.size(), 2);

  Hash candidate_d_hash(hashFromStrData("AA"));
  ASSERT_TRUE(compareVectors(
      tree.hypotheticalDepths(candidate_d_hash,
                              HypotheticalCandidateIncomplete{
                                  .candidate_hash = {},
                                  .candidate_para = 0,
                                  .parent_head_data_hash = hasher_->blake2b_256(
                                      std::vector<uint8_t>{0x0b}),
                                  .candidate_relay_parent = relay_parent_c,
                              },
                              storage,
                              false),
      {1}));
  ASSERT_TRUE(compareVectors(
      tree.hypotheticalDepths(candidate_d_hash,
                              HypotheticalCandidateIncomplete{
                                  .candidate_hash = {},
                                  .candidate_para = 0,
                                  .parent_head_data_hash = hasher_->blake2b_256(
                                      std::vector<uint8_t>{0x0c}),
                                  .candidate_relay_parent = relay_parent_b,
                              },
                              storage,
                              false),
      {2}));
}

TEST_F(ProspectiveParachainsTest,
       Candidates_insertingUnconfirmedRejectsOnIncompatibleClaims) {
  HeadData relay_head_data_a{{1, 2, 3}};
  HeadData relay_head_data_b{{4, 5, 6}};

  const Hash relay_hash_a = hasher_->blake2b_256(relay_head_data_a);
  const Hash relay_hash_b = hasher_->blake2b_256(relay_head_data_b);

  ParachainId para_id_a{1};
  ParachainId para_id_b{2};

  const auto &[candidate_a, pvd_a] = make_candidate(relay_hash_a,
                                                    1,
                                                    para_id_a,
                                                    relay_head_data_a,
                                                    {1},
                                                    hashFromStrData("1000"));
  const Hash candidate_hash_a = network::candidateHash(*hasher_, candidate_a);
  const libp2p::peer::PeerId peer{"peer1"_peerid};

  GroupIndex group_index_a = 100;
  GroupIndex group_index_b = 200;

  Candidates candidates;
  candidates.confirm_candidate(
      candidate_hash_a, candidate_a, pvd_a, group_index_a, hasher_);

  ASSERT_FALSE(
      candidates.insert_unconfirmed(peer,
                                    candidate_hash_a,
                                    relay_hash_b,
                                    group_index_a,
                                    std::make_pair(relay_hash_a, para_id_a)));
  ASSERT_FALSE(
      candidates.insert_unconfirmed(peer,
                                    candidate_hash_a,
                                    relay_hash_a,
                                    group_index_b,
                                    std::make_pair(relay_hash_a, para_id_a)));
  ASSERT_FALSE(
      candidates.insert_unconfirmed(peer,
                                    candidate_hash_a,
                                    relay_hash_a,
                                    group_index_a,
                                    std::make_pair(relay_hash_b, para_id_a)));
  ASSERT_FALSE(
      candidates.insert_unconfirmed(peer,
                                    candidate_hash_a,
                                    relay_hash_a,
                                    group_index_a,
                                    std::make_pair(relay_hash_a, para_id_b)));
  ASSERT_TRUE(
      candidates.insert_unconfirmed(peer,
                                    candidate_hash_a,
                                    relay_hash_a,
                                    group_index_a,
                                    std::make_pair(relay_hash_a, para_id_a)));
}

TEST_F(ProspectiveParachainsTest,
       Candidates_confirmingMaintainsParentHashIndex) {
  HeadData relay_head_data{{1, 2, 3}};
  const Hash relay_hash = hasher_->blake2b_256(relay_head_data);

  HeadData candidate_head_data_a{1};
  HeadData candidate_head_data_b{2};
  HeadData candidate_head_data_c{3};
  HeadData candidate_head_data_d{4};

  const Hash candidate_head_data_hash_a =
      hasher_->blake2b_256(candidate_head_data_a);
  const Hash candidate_head_data_hash_b =
      hasher_->blake2b_256(candidate_head_data_b);
  const Hash candidate_head_data_hash_c =
      hasher_->blake2b_256(candidate_head_data_c);

  const auto &[candidate_a, pvd_a] = make_candidate(relay_hash,
                                                    1,
                                                    1,
                                                    relay_head_data,
                                                    candidate_head_data_a,
                                                    hashFromStrData("1000"));
  const auto &[candidate_b, pvd_b] = make_candidate(relay_hash,
                                                    1,
                                                    1,
                                                    candidate_head_data_a,
                                                    candidate_head_data_b,
                                                    hashFromStrData("2000"));
  const auto &[candidate_c, pvd_c] = make_candidate(relay_hash,
                                                    1,
                                                    1,
                                                    candidate_head_data_b,
                                                    candidate_head_data_c,
                                                    hashFromStrData("3000"));
  const auto &[candidate_d, pvd_d] = make_candidate(relay_hash,
                                                    1,
                                                    1,
                                                    candidate_head_data_c,
                                                    candidate_head_data_d,
                                                    hashFromStrData("4000"));

  const Hash candidate_hash_a = network::candidateHash(*hasher_, candidate_a);
  const Hash candidate_hash_b = network::candidateHash(*hasher_, candidate_b);
  const Hash candidate_hash_c = network::candidateHash(*hasher_, candidate_c);
  const Hash candidate_hash_d = network::candidateHash(*hasher_, candidate_d);

  const libp2p::peer::PeerId peer{"peer1"_peerid};
  GroupIndex group_index = 100;

  Candidates candidates;
  ASSERT_TRUE(candidates.insert_unconfirmed(
      peer, candidate_hash_a, relay_hash, group_index, std::nullopt));
  ASSERT_TRUE(candidates.by_parent.empty());

  ASSERT_TRUE(candidates.insert_unconfirmed(peer,
                                            candidate_hash_a,
                                            relay_hash,
                                            group_index,
                                            std::make_pair(relay_hash, 1)));
  ASSERT_TRUE(compareMapsOfCandidates(
      candidates.by_parent, {{relay_hash, {{1, {candidate_hash_a}}}}}));

  ASSERT_TRUE(candidates.insert_unconfirmed(
      peer,
      candidate_hash_b,
      relay_hash,
      group_index,
      std::make_pair(candidate_head_data_hash_a, 1)));
  ASSERT_TRUE(compareMapsOfCandidates(
      candidates.by_parent,
      {{relay_hash, {{1, {candidate_hash_a}}}},
       {candidate_head_data_hash_a, {{1, {candidate_hash_b}}}}}));

  ASSERT_TRUE(candidates.insert_unconfirmed(
      peer,
      candidate_hash_c,
      relay_hash,
      group_index,
      std::make_pair(candidate_head_data_hash_a, 1)));
  ASSERT_TRUE(
      compareMapsOfCandidates(candidates.by_parent,
                              {{relay_hash, {{1, {candidate_hash_a}}}},
                               {candidate_head_data_hash_a,
                                {{1, {candidate_hash_b, candidate_hash_c}}}}}));

  ASSERT_TRUE(candidates.insert_unconfirmed(
      peer,
      candidate_hash_d,
      relay_hash,
      group_index,
      std::make_pair(candidate_head_data_hash_a, 1)));
  ASSERT_TRUE(compareMapsOfCandidates(
      candidates.by_parent,
      {{relay_hash, {{1, {candidate_hash_a}}}},
       {candidate_head_data_hash_a,
        {{1, {candidate_hash_b, candidate_hash_c, candidate_hash_d}}}}}));

  candidates.confirm_candidate(
      candidate_hash_a, candidate_a, pvd_a, group_index, hasher_);
  ASSERT_TRUE(compareMapsOfCandidates(
      candidates.by_parent,
      {{relay_hash, {{1, {candidate_hash_a}}}},
       {candidate_head_data_hash_a,
        {{1, {candidate_hash_b, candidate_hash_c, candidate_hash_d}}}}}));

  candidates.confirm_candidate(
      candidate_hash_b, candidate_b, pvd_b, group_index, hasher_);
  ASSERT_TRUE(compareMapsOfCandidates(
      candidates.by_parent,
      {{relay_hash, {{1, {candidate_hash_a}}}},
       {candidate_head_data_hash_a,
        {{1, {candidate_hash_b, candidate_hash_c, candidate_hash_d}}}}}));

  candidates.confirm_candidate(
      candidate_hash_d, candidate_d, pvd_d, group_index, hasher_);
  ASSERT_TRUE(compareMapsOfCandidates(
      candidates.by_parent,
      {{relay_hash, {{1, {candidate_hash_a}}}},
       {candidate_head_data_hash_a,
        {{1, {candidate_hash_b, candidate_hash_c}}}},
       {candidate_head_data_hash_c, {{1, {candidate_hash_d}}}}}));

  const auto &[new_candidate_c, new_pvd_c] =
      make_candidate(relay_hash,
                     1,
                     2,
                     candidate_head_data_b,
                     candidate_head_data_c,
                     hashFromStrData("3000"));
  candidates.confirm_candidate(
      candidate_hash_c, new_candidate_c, new_pvd_c, group_index, hasher_);
  ASSERT_TRUE(compareMapsOfCandidates(
      candidates.by_parent,
      {{relay_hash, {{1, {candidate_hash_a}}}},
       {candidate_head_data_hash_a, {{1, {candidate_hash_b}}}},
       {candidate_head_data_hash_b, {{2, {candidate_hash_c}}}},
       {candidate_head_data_hash_c, {{1, {candidate_hash_d}}}}}));
}

TEST_F(ProspectiveParachainsTest, Candidates_testReturnedPostConfirmation) {
  HeadData relay_head_data{{1, 2, 3}};
  const Hash relay_hash = hasher_->blake2b_256(relay_head_data);

  HeadData candidate_head_data_a{1};
  HeadData candidate_head_data_b{2};
  HeadData candidate_head_data_c{3};
  HeadData candidate_head_data_d{4};

  const Hash candidate_head_data_hash_a =
      hasher_->blake2b_256(candidate_head_data_a);
  const Hash candidate_head_data_hash_b =
      hasher_->blake2b_256(candidate_head_data_b);

  const auto &[candidate_a, pvd_a] = make_candidate(relay_hash,
                                                    1,
                                                    1,
                                                    relay_head_data,
                                                    candidate_head_data_a,
                                                    hashFromStrData("1000"));
  const auto &[candidate_b, pvd_b] = make_candidate(relay_hash,
                                                    1,
                                                    1,
                                                    candidate_head_data_a,
                                                    candidate_head_data_b,
                                                    hashFromStrData("2000"));
  const auto &[candidate_c, _] = make_candidate(relay_hash,
                                                1,
                                                1,
                                                candidate_head_data_a,
                                                candidate_head_data_c,
                                                hashFromStrData("3000"));
  const auto &[candidate_d, pvd_d] = make_candidate(relay_hash,
                                                    1,
                                                    1,
                                                    candidate_head_data_b,
                                                    candidate_head_data_d,
                                                    hashFromStrData("4000"));

  const Hash candidate_hash_a = network::candidateHash(*hasher_, candidate_a);
  const Hash candidate_hash_b = network::candidateHash(*hasher_, candidate_b);
  const Hash candidate_hash_c = network::candidateHash(*hasher_, candidate_c);
  const Hash candidate_hash_d = network::candidateHash(*hasher_, candidate_d);

  const libp2p::peer::PeerId peer_a{"peer1"_peerid};
  const libp2p::peer::PeerId peer_b{"peer2"_peerid};
  const libp2p::peer::PeerId peer_c{"peer3"_peerid};
  const libp2p::peer::PeerId peer_d{"peer4"_peerid};

  GroupIndex group_index = 100;
  Candidates candidates;

  ASSERT_TRUE(candidates.insert_unconfirmed(
      peer_a, candidate_hash_a, relay_hash, group_index, std::nullopt));
  ASSERT_TRUE(candidates.insert_unconfirmed(peer_a,
                                            candidate_hash_a,
                                            relay_hash,
                                            group_index,
                                            std::make_pair(relay_hash, 1)));
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

  ASSERT_TRUE(compareMapsOfCandidates(
      candidates.by_parent,
      {{relay_hash, {{1, {candidate_hash_a}}}},
       {candidate_head_data_hash_a,
        {{1, {candidate_hash_b, candidate_hash_c, candidate_hash_d}}}},
       {candidate_head_data_hash_b, {{1, {candidate_hash_d}}}}}));

  {
    auto post_confirmation = candidates.confirm_candidate(
        candidate_hash_a, candidate_a, pvd_a, group_index, hasher_);
    ASSERT_TRUE(post_confirmation);
    PostConfirmation pc{
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
    };
    ASSERT_EQ(*post_confirmation, pc);
  }
  {
    auto post_confirmation = candidates.confirm_candidate(
        candidate_hash_b, candidate_b, pvd_b, group_index, hasher_);
    ASSERT_TRUE(post_confirmation);
    PostConfirmation pc{
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
    };
    ASSERT_EQ(*post_confirmation, pc);
  }

  const auto &[new_candidate_c, new_pvd_c] =
      make_candidate(relay_hash,
                     1,
                     2,
                     candidate_head_data_b,
                     candidate_head_data_c,
                     hashFromStrData("3000"));
  {
    auto post_confirmation = candidates.confirm_candidate(
        candidate_hash_c, new_candidate_c, new_pvd_c, group_index, hasher_);
    ASSERT_TRUE(post_confirmation);
    PostConfirmation pc{
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
    };
    ASSERT_EQ(*post_confirmation, pc);
  }
  {
    auto post_confirmation = candidates.confirm_candidate(
        candidate_hash_d, candidate_d, pvd_d, group_index, hasher_);
    ASSERT_TRUE(post_confirmation);
    PostConfirmation pc{
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
    };
    ASSERT_EQ(*post_confirmation, pc);
  }
}

TEST_F(ProspectiveParachainsTest, Candidates_testHypotheticalFrontiers) {
  HeadData relay_head_data{{1, 2, 3}};
  const Hash relay_hash = hasher_->blake2b_256(relay_head_data);

  HeadData candidate_head_data_a{1};
  HeadData candidate_head_data_b{2};
  HeadData candidate_head_data_c{3};
  HeadData candidate_head_data_d{4};

  const Hash candidate_head_data_hash_a =
      hasher_->blake2b_256(candidate_head_data_a);
  const Hash candidate_head_data_hash_b =
      hasher_->blake2b_256(candidate_head_data_b);
  const Hash candidate_head_data_hash_d =
      hasher_->blake2b_256(candidate_head_data_d);

  const auto &[candidate_a, pvd_a] = make_candidate(relay_hash,
                                                    1,
                                                    1,
                                                    relay_head_data,
                                                    candidate_head_data_a,
                                                    hashFromStrData("1000"));
  const auto &[candidate_b, _] = make_candidate(relay_hash,
                                                1,
                                                1,
                                                candidate_head_data_a,
                                                candidate_head_data_b,
                                                hashFromStrData("2000"));
  const auto &[candidate_c, __] = make_candidate(relay_hash,
                                                 1,
                                                 1,
                                                 candidate_head_data_a,
                                                 candidate_head_data_c,
                                                 hashFromStrData("3000"));
  const auto &[candidate_d, ___] = make_candidate(relay_hash,
                                                  1,
                                                  1,
                                                  candidate_head_data_b,
                                                  candidate_head_data_d,
                                                  hashFromStrData("4000"));

  const Hash candidate_hash_a = network::candidateHash(*hasher_, candidate_a);
  const Hash candidate_hash_b = network::candidateHash(*hasher_, candidate_b);
  const Hash candidate_hash_c = network::candidateHash(*hasher_, candidate_c);
  const Hash candidate_hash_d = network::candidateHash(*hasher_, candidate_d);

  const libp2p::peer::PeerId peer{"peer1"_peerid};

  GroupIndex group_index = 100;
  Candidates candidates;

  candidates.confirm_candidate(
      candidate_hash_a, candidate_a, pvd_a, group_index, hasher_);

  ASSERT_TRUE(candidates.insert_unconfirmed(
      peer,
      candidate_hash_b,
      relay_hash,
      group_index,
      std::make_pair(candidate_head_data_hash_a, 1)));
  ASSERT_TRUE(candidates.insert_unconfirmed(
      peer,
      candidate_hash_c,
      relay_hash,
      group_index,
      std::make_pair(candidate_head_data_hash_a, 1)));
  ASSERT_TRUE(candidates.insert_unconfirmed(
      peer,
      candidate_hash_d,
      relay_hash,
      group_index,
      std::make_pair(candidate_head_data_hash_b, 1)));

  ASSERT_TRUE(compareMapsOfCandidates(
      candidates.by_parent,
      {{relay_hash, {{1, {candidate_hash_a}}}},
       {candidate_head_data_hash_a,
        {{1, {candidate_hash_b, candidate_hash_c}}}},
       {candidate_head_data_hash_b, {{1, {candidate_hash_d}}}}}));

  HypotheticalCandidateComplete hypothetical_a{
      .candidate_hash = candidate_hash_a,
      .receipt = candidate_a,
      .persisted_validation_data = pvd_a,
  };
  HypotheticalCandidateIncomplete hypothetical_b{
      .candidate_hash = candidate_hash_b,
      .candidate_para = 1,
      .parent_head_data_hash = candidate_head_data_hash_a,
      .candidate_relay_parent = relay_hash,
  };
  HypotheticalCandidateIncomplete hypothetical_c{
      .candidate_hash = candidate_hash_c,
      .candidate_para = 1,
      .parent_head_data_hash = candidate_head_data_hash_a,
      .candidate_relay_parent = relay_hash,
  };
  HypotheticalCandidateIncomplete hypothetical_d{
      .candidate_hash = candidate_hash_d,
      .candidate_para = 1,
      .parent_head_data_hash = candidate_head_data_hash_b,
      .candidate_relay_parent = relay_hash,
  };

  {
    auto hypotheticals =
        candidates.frontier_hypotheticals(std::make_pair(relay_hash, 1));
    ASSERT_EQ(hypotheticals.size(), 1);
    ASSERT_TRUE(std::find(hypotheticals.begin(),
                          hypotheticals.end(),
                          HypotheticalCandidate{hypothetical_a})
                != hypotheticals.end());
  }
  {
    auto hypotheticals = candidates.frontier_hypotheticals(
        std::make_pair(candidate_head_data_hash_a, 2));
    ASSERT_EQ(hypotheticals.size(), 0);
  }
  {
    auto hypotheticals = candidates.frontier_hypotheticals(
        std::make_pair(candidate_head_data_hash_a, 1));
    ASSERT_EQ(hypotheticals.size(), 2);
    ASSERT_TRUE(std::find(hypotheticals.begin(),
                          hypotheticals.end(),
                          HypotheticalCandidate{hypothetical_b})
                != hypotheticals.end());
    ASSERT_TRUE(std::find(hypotheticals.begin(),
                          hypotheticals.end(),
                          HypotheticalCandidate{hypothetical_c})
                != hypotheticals.end());
  }
  {
    auto hypotheticals = candidates.frontier_hypotheticals(
        std::make_pair(candidate_head_data_hash_d, 1));
    ASSERT_EQ(hypotheticals.size(), 0);
  }
  {
    auto hypotheticals = candidates.frontier_hypotheticals(std::nullopt);
    ASSERT_EQ(hypotheticals.size(), 4);
    ASSERT_TRUE(std::find(hypotheticals.begin(),
                          hypotheticals.end(),
                          HypotheticalCandidate{hypothetical_a})
                != hypotheticals.end());
    ASSERT_TRUE(std::find(hypotheticals.begin(),
                          hypotheticals.end(),
                          HypotheticalCandidate{hypothetical_b})
                != hypotheticals.end());
    ASSERT_TRUE(std::find(hypotheticals.begin(),
                          hypotheticals.end(),
                          HypotheticalCandidate{hypothetical_c})
                != hypotheticals.end());
    ASSERT_TRUE(std::find(hypotheticals.begin(),
                          hypotheticals.end(),
                          HypotheticalCandidate{hypothetical_d})
                != hypotheticals.end());
  }
}
