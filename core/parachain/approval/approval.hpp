/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_APPROVAL_HPP
#define KAGOME_APPROVAL_HPP

#include "common/visitor.hpp"
#include "consensus/babe/common.hpp"
#include "consensus/validation/prepare_transcript.hpp"
#include "outcome/outcome.hpp"
#include "parachain/approval/state.hpp"
#include "parachain/types.hpp"

namespace kagome::parachain::approval {
  /// Validators assigning to check a particular candidate are split up into
  /// tranches. Earlier tranches of validators check first, with later tranches
  /// serving as backup.
  using DelayTranche = uint32_t;

  /// A static context used to compute the Relay VRF story based on the
  /// VRF output included in the header-chain.
  constexpr auto RELAY_VRF_STORY_CONTEXT = "A&V RC-VRF";

  /// A static context used for all relay-vrf-modulo VRFs.
  constexpr auto RELAY_VRF_DELAY_CONTEXT = "A&V DELAY";

  /// A static context used for transcripts indicating assigned availability
  /// core.
  constexpr auto ASSIGNED_CORE_CONTEXT = "A&V ASSIGNED";

  /// A static context associated with producing randomness for a core.
  constexpr auto CORE_RANDOMNESS_CONTEXT = "A&V CORE";

  /// A static context associated with producing randomness for a tranche.
  constexpr auto TRANCHE_RANDOMNESS_CONTEXT = "A&V TRANCHE";

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

  /// An assignment criterion which refers to the candidate under which the
  /// assignment is relevant by block hash.
  struct IndirectAssignmentCert {
    SCALE_TIE(3);

    Hash block_hash;           /// A block hash where the candidate appears.
    ValidatorIndex validator;  /// The validator index.
    AssignmentCert cert;       /// The cert itself.
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
      ApprovalStateTransition const &val) {
    return kagome::visit_in_place(
        val,
        [](RemoteApproval const &v) -> std::optional<ValidatorIndex> {
          return v.validator_ix;
        },
        [](LocalApproval const &v) -> std::optional<ValidatorIndex> {
          return v.validator_ix;
        },
        [](WakeupProcessed const &) -> std::optional<ValidatorIndex> {
          return std::nullopt;
        });
  }
  inline bool is_local_approval(ApprovalStateTransition const &val) {
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
    consensus::babe::BabeSlotNumber slot;  /// The consensus slot of the block.
    SessionIndex session;                  /// The session of the block.
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
    consensus::babe::BabeSlotNumber slot;
    primitives::AuthorityIndex authority_index;

    /// Get the slot.
    consensus::babe::BabeSlotNumber getSlot() const {
      return slot;
    }

    /// Compute the randomness associated with this VRF output.
    outcome::result<void> compute_randomness(
        ::RelayVRFStory &vrf_story,
        const primitives::AuthorityList &authorities,
        const consensus::babe::Randomness &randomness,
        consensus::babe::EpochNumber epoch_index);
  };

}  // namespace kagome::parachain::approval

OUTCOME_HPP_DECLARE_ERROR(kagome::parachain::approval, UnsafeVRFOutput::Error);

#endif  // KAGOME_APPROVAL_HPP
