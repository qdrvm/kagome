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
    hasher_ = std::make_shared<kagome::crypto::HasherImpl>();
    /// OUTCOME_TRY(enc_header, testutil::scaleEncodeAndCompareWithRef(header));
    /// auto hash = hasher_->blake2b_256(enc_header);
  }

 protected:
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
                .para_head_hash =
                    crypto::Hashed<const HeadData &,
                                   32,
                                   crypto::Blake2b_StreamHasher<32>>{para_head}
                        .getHash(),
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
