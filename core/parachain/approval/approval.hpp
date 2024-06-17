/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/visitor.hpp"
#include "consensus/timeline/types.hpp"
#include "outcome/outcome.hpp"
#include "parachain/approval/state.hpp"
#include "parachain/types.hpp"

namespace kagome::parachain::approval {
  /// Validators assigning to check a particular candidate are split up into
  /// tranches. Earlier tranches of validators check first, with later tranches
  /// serving as backup.
  using DelayTranche = uint32_t;

  /// An assignment story based on the VRF that authorized the relay-chain block
  /// where the candidate was included combined with a sample number.
  ///
  /// The context used to produce bytes is [`RELAY_VRF_MODULO_CONTEXT`]
  struct RelayVRFModulo {
    SCALE_TIE(1);
    uint32_t sample;  /// The sample number used in this cert.
  };

  /// An assignment story based on the VRF that authorized the relay-chain block
  /// where the candidate was included combined with the index of a particular
  /// core.
  ///
  /// The context is [`RELAY_VRF_DELAY_CONTEXT`]
  struct RelayVRFDelay {
    SCALE_TIE(1);
    CoreIndex core_index;  /// The core index chosen in this cert.
  };

  /// random bytes derived from the VRF submitted within the block by the
  /// block author as a credential and used as input to approval assignment
  /// criteria.
  struct RelayVRFStory {
    SCALE_TIE(1);
    ConstBuffer data;
  };

  /// Multiple assignment stories based on the VRF that authorized the
  /// relay-chain block where the candidates were included.
  struct RelayVRFModuloCompact {
    SCALE_TIE(1);
    /// A bitfield representing the core indices claimed by this assignment.
    scale::BitVec core_bitfield;
  };

  /// Different kinds of input data or criteria that can prove a validator's
  /// assignment to check a particular parachain.
  using AssignmentCertKind = boost::variant<RelayVRFModulo, RelayVRFDelay>;

  /// A certification of assignment.
  struct AssignmentCert {
    SCALE_TIE(2);

    AssignmentCertKind
        kind;  /// The criterion which is claimed to be met by this cert.
    crypto::VRFOutput vrf;  /// The VRF showing the criterion is met.
  };

  /// Certificate is changed compared to `AssignmentCertKind`:
  /// - introduced RelayVRFModuloCompact
  using AssignmentCertKindV2 =
      boost::variant<RelayVRFModuloCompact, RelayVRFDelay, RelayVRFModulo>;

  /// A certification of assignment.
  struct AssignmentCertV2 {
    SCALE_TIE(2);

    /// The criterion which is claimed to be met by this cert.
    AssignmentCertKindV2 kind;

    /// The VRF showing the criterion is met.
    crypto::VRFOutput vrf;

    static AssignmentCertV2 from(const AssignmentCert &src) {
      return AssignmentCertV2 {
        .kind = visit_in_place(
          src.kind,
          [](const auto &v) -> AssignmentCertKindV2 {
            return v;
          }),
        .vrf = src.vrf,
      };
    }
  };

  /// An assignment criterion which refers to the candidate under which the
  /// assignment is relevant by block hash.
  struct IndirectAssignmentCert {
    SCALE_TIE(3);

    Hash block_hash;           /// A block hash where the candidate appears.
    ValidatorIndex validator;  /// The validator index.
    AssignmentCert cert;       /// The cert itself.
  };

  /// An assignment criterion which refers to the candidate under which the
  /// assignment is relevant by block hash.
  struct IndirectAssignmentCertV2 {
    SCALE_TIE(3);

    /// A block hash where the candidate appears.
    Hash block_hash;

    /// The validator index.
    ValidatorIndex validator;

    /// The cert itself.
    AssignmentCertV2 cert;

    static IndirectAssignmentCertV2 from(const IndirectAssignmentCert &src) {
      return IndirectAssignmentCertV2 {
        .block_hash = src.block_hash,
        .validator = src.validator,
        .cert = AssignmentCertV2::from(src.cert),
      };
    }
  };

  /// A signed approval vote which references the candidate indirectly via the
  /// block.
  ///
  /// In practice, we have a look-up from block hash and candidate index to
  /// candidate hash, so this can be transformed into a `SignedApprovalVote`.
  struct IndirectApprovalVote {
    SCALE_TIE(2);

    Hash block_hash;  /// A block hash where the candidate appears.
    CandidateIndex
        candidate_index;  /// The index of the candidate in the list of
                          /// candidates fully included as-of the block.
  };
  using IndirectSignedApprovalVote = IndexedAndSigned<IndirectApprovalVote>;

  /// A signed approval vote which references the candidate indirectly via the
  /// block.
  ///
  /// In practice, we have a look-up from block hash and candidate index to
  /// candidate hash, so this can be transformed into a `SignedApprovalVote`.
  struct IndirectApprovalVoteV2 {
    SCALE_TIE(2);

    /// A block hash where the candidate appears.
    Hash block_hash;

    /// The index of the candidate in the list of candidates fully included
    /// as-of the block.
    scale::BitVec candidate_indices;

    static IndirectApprovalVoteV2 from(const IndirectApprovalVote &value) {
      scale::BitVec v;
      v.bits.resize(value.candidate_index + 1);
      v.bits[value.candidate_index] = true;
      return IndirectApprovalVoteV2 {
        .block_hash = value.block_hash,
        .candidate_indices = std::move(v),
      };
    }
  };
  using IndirectSignedApprovalVoteV2 = IndexedAndSigned<IndirectApprovalVoteV2>;

  inline IndirectSignedApprovalVoteV2 from(const IndirectSignedApprovalVote &value) {
    return IndirectSignedApprovalVoteV2 {
      .payload = {
        .payload = IndirectApprovalVoteV2::from(value.payload.payload),
        .ix = value.payload.ix,
      },
      .signature = value.signature,
    };
  }

  struct RemoteApproval {
    ValidatorIndex validator_ix;
  };

  struct LocalApproval {
    ValidatorIndex validator_ix;
    ValidatorSignature validator_sig;
  };

  struct WakeupProcessed {};

  using ApprovalStateTransition =
      boost::variant<RemoteApproval, LocalApproval, WakeupProcessed>;
  inline std::optional<ValidatorIndex> validator_index(
      const ApprovalStateTransition &val) {
    return kagome::visit_in_place(
        val,
        [](const RemoteApproval &v) -> std::optional<ValidatorIndex> {
          return v.validator_ix;
        },
        [](const LocalApproval &v) -> std::optional<ValidatorIndex> {
          return v.validator_ix;
        },
        [](const WakeupProcessed &) -> std::optional<ValidatorIndex> {
          return std::nullopt;
        });
  }
  inline bool is_local_approval(const ApprovalStateTransition &val) {
    return boost::get<LocalApproval>(&val) != nullptr;
  }

  /// Metadata about a block which is now live in the approval protocol.
  struct BlockApprovalMeta {
    primitives::BlockHash hash;         /// The hash of the block.
    primitives::BlockNumber number;     /// The number of the block.
    primitives::BlockHash parent_hash;  /// The hash of the parent block.
    std::vector<CandidateHash>
        candidates;  /// The candidates included by the block. Note that these
    /// are not the same as the candidates that appear within
    /// the block body.
    consensus::SlotNumber slot;  /// The consensus slot of the block.
    SessionIndex session;        /// The session of the block.
  };

  struct ApprovalStatus {
    RequiredTranches required_tranches;
    DelayTranche tranche_now;
    Tick block_tick;
  };

  /// An unsafe VRF output. Provide BABE Epoch info to create a `RelayVRFStory`.
  struct UnsafeVRFOutput {
    enum class Error { AuthorityOutOfBounds = 1, ComputeRandomnessFailed };

    std::reference_wrapper<crypto::VRFOutput> vrf_output;
    consensus::SlotNumber slot;
    consensus::AuthorityIndex authority_index;

    /// Get the slot.
    consensus::SlotNumber getSlot() const {
      return slot;
    }

    /// Compute the randomness associated with this VRF output.
    outcome::result<void> compute_randomness(
        ::RelayVRFStory &vrf_story,
        const consensus::babe::Authorities &authorities,
        const consensus::Randomness &randomness,
        consensus::EpochNumber epoch_index);
  };

}  // namespace kagome::parachain::approval

OUTCOME_HPP_DECLARE_ERROR(kagome::parachain::approval, UnsafeVRFOutput::Error);
