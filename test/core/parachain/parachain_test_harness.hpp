/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include <qtils/test/outcome.hpp>

#include "crypto/hasher/hasher_impl.hpp"
#include "crypto/sr25519/sr25519_provider_impl.hpp"
#include "crypto/type_hasher.hpp"
#include "mock/core/blockchain/block_tree_mock.hpp"
#include "mock/core/runtime/parachain_host_mock.hpp"
#include "parachain/types.hpp"
#include "parachain/validator/prospective_parachains/candidate_storage.hpp"
#include "parachain/validator/signer.hpp"
#include "runtime/runtime_api/parachain_host_types.hpp"
#include "testutil/literals.hpp"
#include "testutil/prepare_loggers.hpp"

using namespace kagome::primitives;
using namespace kagome::parachain;

namespace network = kagome::network;
namespace runtime = kagome::runtime;
namespace common = kagome::common;
namespace crypto = kagome::crypto;

using testing::Return;

inline Hash ghashFromStrData(
    const std::shared_ptr<kagome::crypto::Hasher> &hasher,
    std::span<const char> data) {
  return hasher->blake2b_256(data);
}

class ProspectiveParachainsTestHarness : public testing::Test {
 protected:
  void SetUp() override {
    testutil::prepareLoggers();
    hasher_ = std::make_shared<kagome::crypto::HasherImpl>();

    block_tree_ = std::make_shared<kagome::blockchain::BlockTreeMock>();
    sr25519_provider_ = std::make_shared<crypto::Sr25519ProviderImpl>();
  }

  void TearDown() override {
    block_tree_.reset();
  }

  using CandidatesHashMap = std::unordered_map<
      Hash,
      std::unordered_map<ParachainId, std::unordered_set<CandidateHash>>>;

  std::shared_ptr<kagome::crypto::Hasher> hasher_;
  std::shared_ptr<kagome::blockchain::BlockTreeMock> block_tree_;
  std::shared_ptr<crypto::Sr25519Provider> sr25519_provider_;

  static constexpr uint64_t ALLOWED_ANCESTRY_LEN = 3ull;
  static constexpr uint32_t MAX_POV_SIZE = 1000000;
  static constexpr uint32_t LEGACY_MIN_BACKING_VOTES = 2;

  Hash hashFromStrData(std::span<const char> data) {
    return ghashFromStrData(hasher_, data);
  }

  Hash hash(const network::CommittedCandidateReceipt &data) {
    return network::candidateHash(*hasher_, data);
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
        .validation_code_hash = fromNumber(42),
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
            .relay_parent_storage_root = fromNumber(0),
            .max_pov_size = 1000000,
        });

    network::CommittedCandidateReceipt candidate{
        .descriptor =
            network::CandidateDescriptor{
                .para_id = para_id,
                .relay_parent = relay_parent,
                .reserved_1 = {},
                .persisted_data_hash = persisted_validation_data.getHash(),
                .pov_hash = fromNumber(1),
                .erasure_encoding_root = fromNumber(1),
                .reserved_2 = {},
                .para_head_hash = hasher_->blake2b_256(para_head),
                .validation_code_hash = fromNumber(42),
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

  HeadData dummy_head_data() {
    return {};
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

  network::CandidateDescriptor dummy_candidate_descriptor(
      const Hash &relay_parent) {
    return network::CandidateDescriptor{
        .para_id = 1,
        .relay_parent = relay_parent,
        .reserved_1 = {},
        .persisted_data_hash = {},
        .pov_hash = {},
        .erasure_encoding_root = {},
        .reserved_2 = {},
        .para_head_hash = {},
        .validation_code_hash = {},
    };
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

  runtime::PersistedValidationData dummy_pvd(const HeadData &parent_head,
                                             uint32_t relay_parent_number) {
    return runtime::PersistedValidationData{
        .parent_head = parent_head,
        .relay_parent_number = relay_parent_number,
        .relay_parent_storage_root = {},
        .max_pov_size = MAX_POV_SIZE,
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

  static Hash fromNumber(uint64_t n) {
    uint8_t byte_value = n % 256;
    Hash h{};
    memset(&h[0], byte_value, 32);
    return h;
  }

  static Hash get_parent_hash(const Hash &parent) {
    const auto val = *(uint8_t *)&parent[0];
    return fromNumber(val + 1);
  }
};
