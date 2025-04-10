/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <boost/variant.hpp>
#include <tuple>
#include <type_traits>
#include <vector>

#include "common/blob.hpp"
#include "common/custom_equality.hpp"
#include "common/visitor.hpp"
#include "consensus/grandpa/common.hpp"
#include "crypto/sr25519_types.hpp"
#include "primitives/block_header.hpp"
#include "primitives/common.hpp"
#include "primitives/digest.hpp"
#include "scale/kagome_scale.hpp"
#include "storage/trie/types.hpp"

namespace kagome::parachain {

  using ConstBuffer = kagome::common::Blob<32>;
  using Hash = common::Hash256;
  using Signature = kagome::crypto::Sr25519Signature;
  using ParachainId = uint32_t;
  using PublicKey = crypto::Sr25519PublicKey;
  using CollatorPublicKey = PublicKey;
  using ValidatorIndex = uint32_t;
  using ValidatorId = crypto::Sr25519PublicKey;
  using UpwardMessage = common::Buffer;
  using ParachainRuntime = common::Buffer;
  using HeadData = common::Buffer;
  using CandidateHash = Hash;
  using RelayHash = Hash;
  using ChunkIndex = uint32_t;
  using ChunkProof = std::vector<common::Buffer>;
  using CandidateIndex = uint32_t;
  using CoreIndex = uint32_t;
  using GroupIndex = uint32_t;
  using CollatorId = CollatorPublicKey;
  using ValidationCodeHash = Hash;
  using BlockNumber = kagome::primitives::BlockNumber;
  using DownwardMessage = common::Buffer;
  using SessionIndex = uint32_t;
  using Tick = uint64_t;

  /// Validators assigning to check a particular candidate are split up into
  /// tranches. Earlier tranches of validators check first, with later tranches
  /// serving as backup.
  using DelayTranche = uint32_t;

  /// Signature with which parachain validators sign blocks.
  using ValidatorSignature = Signature;

  template <typename D>
  struct Indexed {
    using Type = std::decay_t<D>;
    Type payload;
    ValidatorIndex ix;
    bool operator==(const Indexed &) const = default;
  };

  /**
   * Appends an index and signature to the given type.
   */
  template <typename T>
  using IndexedAndSigned = kagome::crypto::Sr25519Signed<Indexed<T>>;

  template <typename T>
  [[maybe_unused]] inline const T &getPayload(const IndexedAndSigned<T> &t) {
    return t.payload.payload;
  }

  template <typename T>
  [[maybe_unused]] inline T &getPayload(IndexedAndSigned<T> &t) {
    return t.payload.payload;
  }

  struct PvfCheckStatement {
    bool accept;
    ValidationCodeHash subject;
    SessionIndex session_index;
    ValidatorIndex validator_index;

    auto signable() {
      constexpr std::array<uint8_t, 4> kMagic{'V', 'C', 'P', 'C'};
      return scale::encode(std::make_tuple(kMagic, *this)).value();
    }
  };
}  // namespace kagome::parachain

namespace kagome::network {

  struct OutboundHorizontal {
    /// The para that will get this message in its downward message queue.
    parachain::ParachainId recipient;
    /// The message payload.
    common::Buffer data;
    bool operator==(const OutboundHorizontal &other) const = default;
  };

  struct InboundDownwardMessage {
    /// The block number at which these messages were put into the downward
    /// message queue.
    parachain::BlockNumber sent_at;
    /// The actual downward message to processes.
    parachain::DownwardMessage msg;
    bool operator==(const InboundDownwardMessage &other) const = default;
  };

  struct InboundHrmpMessage {
    /// The block number at which this message was sent.
    /// Specifically, it is the block number at which the candidate that sends
    /// this message was enacted.
    parachain::BlockNumber sent_at;
    /// The message payload.
    common::Buffer data;
    bool operator==(const InboundHrmpMessage &other) const = default;
  };

  struct CandidateCommitments {
    std::vector<parachain::UpwardMessage> upward_msgs;  /// upward messages
    std::vector<OutboundHorizontal>
        outbound_hor_msgs;  /// outbound horizontal messages
    std::optional<parachain::ParachainRuntime>
        opt_para_runtime;           /// new parachain runtime if present
    parachain::HeadData para_head;  /// parachain head data
    uint32_t downward_msgs_count;   /// number of downward messages that were
    /// processed by the parachain
    parachain::BlockNumber
        watermark;  /// watermark which specifies the relay chain block
    /// number up to which all inbound horizontal messages
    /// have been processed
    bool operator==(const CandidateCommitments &other) const = default;
  };

  /**
   * Unique descriptor of a candidate receipt.
   */
  struct CandidateDescriptor {
    parachain::ParachainId para_id;  /// Parachain Id
    primitives::BlockHash
        relay_parent;  /// Hash of the relay chain block the candidate is
    /// executed in the context of
    common::Blob<32> reserved_1;
    primitives::BlockHash
        persisted_data_hash;         /// Hash of the persisted validation data
    primitives::BlockHash pov_hash;  /// Hash of the PoV block.
    storage::trie::RootHash
        erasure_encoding_root;  /// Root of the block’s erasure encoding Merkle
    /// tree.
    common::Blob<64> reserved_2;
    primitives::BlockHash
        para_head_hash;  /// Hash of the parachain head data of this candidate.
    primitives::BlockHash
        validation_code_hash;  /// Hash of the parachain Runtime.

    bool operator==(const CandidateDescriptor &) const = default;

    common::Buffer signable() const {
      return common::Buffer{
          scale::encode(std::tie(relay_parent,
                                 para_id,
                                 persisted_data_hash,
                                 pov_hash,
                                 validation_code_hash))
              .value(),
      };
    }
  };

  /**
   * Contains information about the candidate and a proof of the results of its
   * execution.
   */
  struct CandidateReceipt {
    CandidateDescriptor descriptor;    /// Candidate descriptor
    parachain::Hash commitments_hash;  /// Hash of candidate commitments
    const parachain::Hash &hash(const crypto::Hasher &hasher) const {
      if (not hash_.has_value()) {
        hash_.emplace(hasher.blake2b_256(
            scale::encode(std::tie(descriptor, commitments_hash)).value()));
      }
      return hash_.value();
    }

    CUSTOM_EQUALITY(CandidateReceipt, descriptor, commitments_hash);
    SCALE_CUSTOM_DECOMPOSITION(CandidateReceipt, descriptor, commitments_hash)

   private:
    mutable std::optional<parachain::Hash> hash_{};
  };

  struct CommittedCandidateReceipt {
    CandidateDescriptor descriptor;    /// Candidate descriptor
    CandidateCommitments commitments;  /// commitments retrieved from validation
    /// result and produced by the execution
    /// and validation parachain candidate

    CandidateReceipt to_plain(const crypto::Hasher &hasher) const {
      CandidateReceipt receipt;
      receipt.descriptor = descriptor,
      receipt.commitments_hash =
          hasher.blake2b_256(scale::encode(commitments).value());
      return receipt;
    }

    bool operator==(const CommittedCandidateReceipt &r) const = default;
  };

}  // namespace kagome::network

namespace kagome::parachain::fragment {
  enum UpgradeRestriction : uint8_t {
    /// There is an upgrade restriction and there are no details about its
    /// specifics nor how long
    /// it could last.
    Present = 0,
  };

  struct CandidatePendingAvailability {
    /// The hash of the candidate.
    CandidateHash candidate_hash;
    /// The candidate's descriptor.
    network::CandidateDescriptor descriptor;
    /// The commitments of the candidate.
    network::CandidateCommitments commitments;
    /// The candidate's relay parent's number.
    BlockNumber relay_parent_number;
    /// The maximum Proof-of-Validity size allowed, in bytes.
    uint32_t max_pov_size;
  };

  /// Constraints on inbound HRMP channels.
  struct InboundHrmpLimitations {
    /// An exhaustive set of all valid watermarks, sorted ascending
    std::vector<BlockNumber> valid_watermarks;
  };

  /// Constraints on outbound HRMP channels.
  struct OutboundHrmpChannelLimitations {
    /// The maximum bytes that can be written to the channel.
    uint32_t bytes_remaining;
    /// The maximum messages that can be written to the channel.
    uint32_t messages_remaining;
  };

  struct HrmpWatermarkUpdateHead {
    BlockNumber v;
  };
  struct HrmpWatermarkUpdateTrunk {
    BlockNumber v;
  };
  using HrmpWatermarkUpdate =
      boost::variant<HrmpWatermarkUpdateHead, HrmpWatermarkUpdateTrunk>;
  inline BlockNumber fromHrmpWatermarkUpdate(const HrmpWatermarkUpdate &value) {
    return visit_in_place(value, [](const auto &val) { return val.v; });
  }

  struct OutboundHrmpChannelModification {
    /// The number of bytes submitted to the channel.
    uint32_t bytes_submitted;
    /// The number of messages submitted to the channel.
    uint32_t messages_submitted;
  };

  struct ConstraintModifications {
    /// The required parent head to build upon.
    std::optional<HeadData> required_parent{};
    /// The new HRMP watermark
    std::optional<HrmpWatermarkUpdate> hrmp_watermark{};
    /// Outbound HRMP channel modifications.
    std::unordered_map<ParachainId, OutboundHrmpChannelModification>
        outbound_hrmp{};
    /// The amount of UMP messages sent.
    uint32_t ump_messages_sent{0ull};
    /// The amount of UMP bytes sent.
    uint32_t ump_bytes_sent{0ull};
    /// The amount of DMP messages processed.
    uint32_t dmp_messages_processed{0ull};
    /// Whether a pending code upgrade has been applied.
    bool code_upgrade_applied{false};

    void stack(const ConstraintModifications &other) {
      if (other.required_parent) {
        required_parent = other.required_parent;
      }
      if (other.hrmp_watermark) {
        hrmp_watermark = other.hrmp_watermark;
      }

      for (const auto &[id, mods] : other.outbound_hrmp) {
        auto &record = outbound_hrmp[id];
        record.messages_submitted += mods.messages_submitted;
        record.bytes_submitted += mods.bytes_submitted;
      }

      ump_messages_sent += other.ump_messages_sent;
      ump_bytes_sent += other.ump_bytes_sent;
      dmp_messages_processed += other.dmp_messages_processed;
      code_upgrade_applied |= other.code_upgrade_applied;
    }
  };

  struct Constraints {
    enum class Error {
      DISALLOWED_HRMP_WATERMARK,
      NO_SUCH_HRMP_CHANNEL,
      HRMP_BYTES_OVERFLOW,
      HRMP_MESSAGE_OVERFLOW,
      UMP_MESSAGE_OVERFLOW,
      UMP_BYTES_OVERFLOW,
      DMP_MESSAGE_UNDERFLOW,
      APPLIED_NONEXISTENT_CODE_UPGRADE,
    };

    /// The minimum relay-parent number accepted under these constraints.
    BlockNumber min_relay_parent_number;
    /// The maximum Proof-of-Validity size allowed, in bytes.
    uint32_t max_pov_size;
    /// The maximum new validation code size allowed, in bytes.
    uint32_t max_code_size;
    /// The amount of UMP messages remaining.
    uint32_t ump_remaining;
    /// The amount of UMP bytes remaining.
    uint32_t ump_remaining_bytes;
    /// The maximum number of UMP messages allowed per candidate.
    uint32_t max_ump_num_per_candidate;
    /// Remaining DMP queue. Only includes sent-at block numbers.
    std::vector<BlockNumber> dmp_remaining_messages;
    /// The limitations of all registered inbound HRMP channels.
    InboundHrmpLimitations hrmp_inbound;
    /// The limitations of all registered outbound HRMP channels.
    std::unordered_map<ParachainId, OutboundHrmpChannelLimitations>
        hrmp_channels_out;
    /// The maximum number of HRMP messages allowed per candidate.
    uint32_t max_hrmp_num_per_candidate;
    /// The required parent head-data of the parachain.
    HeadData required_parent;
    /// The expected validation-code-hash of this parachain.
    ValidationCodeHash validation_code_hash;
    /// The code upgrade restriction signal as-of this parachain.
    std::optional<UpgradeRestriction> upgrade_restriction;
    /// The future validation code hash, if any, and at what relay-parent
    /// number the upgrade would be minimally applied.
    std::optional<std::pair<BlockNumber, ValidationCodeHash>>
        future_validation_code;

    outcome::result<Constraints> apply_modifications(
        const ConstraintModifications &modifications) const;

    outcome::result<void> check_modifications(
        const ConstraintModifications &modifications) const;
  };

  struct BackingState {
    /// The state-machine constraints of the parachain.
    Constraints constraints;
    /// The candidates pending availability. These should be ordered, i.e. they
    /// should form a sub-chain, where the first candidate builds on top of the
    /// required parent of the constraints and each subsequent builds on top of
    /// the previous head-data.
    std::vector<CandidatePendingAvailability> pending_availability;
  };

}  // namespace kagome::parachain::fragment

OUTCOME_HPP_DECLARE_ERROR(kagome::parachain::fragment, Constraints::Error);
