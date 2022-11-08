/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_PARACHAIN_HOST_TYPES_HPP
#define KAGOME_CORE_RUNTIME_PARACHAIN_HOST_TYPES_HPP

#include <scale/bitvec.hpp>

#include "common/blob.hpp"
#include "common/unused.hpp"
#include "primitives/authority_discovery_id.hpp"
#include "primitives/block_id.hpp"
#include "primitives/common.hpp"
#include "primitives/parachain_host.hpp"

namespace kagome::runtime {

  using Buffer = common::Buffer;
  using ValidatorId = network::ValidatorId;
  using DutyRoster = primitives::parachain::DutyRoster;
  using ParachainId = network::ParachainId;
  using GroupIndex = network::GroupIndex;
  using CollatorId = network::CollatorId;
  using Hash = network::Hash;
  using CollatorSignature = network::Signature;
  using ValidationCodeHash = network::Hash;
  using BlockNumber = network::BlockNumber;
  using CandidateHash = network::CandidateHash;
  using HeadData = network::HeadData;
  using GroupRotatePeriod = uint32_t;
  using UpwardMessage = network::UpwardMessage;
  using ValidationCode = Buffer;
  using SessionIndex = network::SessionIndex;
  using CoreIndex = network::CoreIndex;
  using ScheduledCore = network::ScheduledCore;
  using CandidateDescriptor = network::CandidateDescriptor;

  /// Information about a core which is currently occupied.
  struct OccupiedCore {
    // NOTE: this has no ParaId as it can be deduced from the candidate
    // descriptor.
    /// If this core is freed by availability, this is the assignment that is
    /// next up on this core, if any. None if there is nothing queued for this
    /// core.
    std::optional<ScheduledCore> next_up_on_available;
    /// The relay-chain block number this began occupying the core at.
    BlockNumber occupied_since;
    /// The relay-chain block this will time-out at, if any.
    BlockNumber time_out_at;
    /// If this core is freed by being timed-out, this is the assignment that is
    /// next up on this core. None if there is nothing queued for this core or
    /// there is no possibility of timing out.
    std::optional<ScheduledCore> next_up_on_time_out;
    /// A bitfield with 1 bit for each validator in the set. `1` bits mean that
    /// the corresponding validators has attested to availability on-chain. A
    /// 2/3+ majority of `1` bits means that this will be available.
    scale::BitVec availability;
    /// The group assigned to distribute availability pieces of this candidate.
    GroupIndex group_responsible;
    /// The hash of the candidate occupying the core.
    CandidateHash candidate_hash;
    /// The descriptor of the candidate occupying the core.
    CandidateDescriptor candidate_descriptor;
  };

  using GroupDescriptor =
      std::tuple<BlockNumber, GroupRotatePeriod, BlockNumber>;
  using ValidatorGroup = std::tuple<std::vector<ValidatorId>, GroupDescriptor>;
  using CoreState = boost::variant<OccupiedCore,   // 0
                                   ScheduledCore,  // 1
                                   Unused<2>>;     // 2
  enum class OccupiedCoreAssumption {
    Included,  // 0
    TimedOut,  // 1
    Unused     // 2
  };
  struct PersistedValidationData {
    /// The parent head-data.
    HeadData parent_head;
    /// The relay-chain block number this is in the context of.
    BlockNumber relay_parent_number;
    /// The relay-chain block storage root this is in the context of.
    Hash relay_parent_storage_root;
    /// The maximum legal size of a POV block, in bytes.
    uint32_t max_pov_size;
  };

  using OutboundHrmpMessage = network::OutboundHorizontal;
  using CandidateCommitments = network::CandidateCommitments;
  using CommittedCandidateReceipt = network::CommittedCandidateReceipt;
  using CandidateReceipt = network::CandidateReceipt;

  struct Candidate {
    CandidateReceipt candidate_receipt;
    HeadData head_data;
    CoreIndex core_index;
  };

  struct CandidateBacked : public Candidate {
    GroupIndex group_index{};
  };

  struct CandidateIncluded : public Candidate {
    GroupIndex group_index{};
  };

  struct CandidateTimedOut : public Candidate {};

  using CandidateEvent = boost::variant<
      /// This candidate receipt was backed in the most recent block.
      /// This includes the core index the candidate is now occupying.
      CandidateBacked,  // 0
      /// This candidate receipt was included and became a parablock at the
      /// most recent block. This includes the core index the candidate was
      /// occupying as well as the group responsible for backing the
      /// candidate.
      CandidateIncluded,  // 1
      /// This candidate receipt was not made available in time and timed out.
      /// This includes the core index the candidate was occupying.
      CandidateTimedOut  // 2
      >;

  using ValidatorIndex = network::ValidatorIndex;
  using AuthorityDiscoveryId = common::Hash256;
  using AssignmentId = common::Blob<32>;
  struct SessionInfo {
    /****** New in v2 *******/
    /// All the validators actively participating in parachain consensus.
    /// Indices are into the broader validator set.
    std::vector<ValidatorIndex> active_validator_indices;
    /// A secure random seed for the session, gathered from BABE.
    common::Blob<32> random_seed;
    /// The amount of sessions to keep for disputes.
    SessionIndex dispute_period;

    /****** Old fields ******/
    /// Validators in canonical ordering.
    ///
    /// NOTE: There might be more authorities in the current session, than
    /// `validators` participating in parachain consensus. See
    /// [`max_validators`](https://github.com/paritytech/polkadot/blob/a52dca2be7840b23c19c153cf7e110b1e3e475f8/runtime/parachains/src/configuration.rs#L148).
    ///
    /// `SessionInfo::validators` will be limited to to `max_validators` when
    /// set.
    std::vector<ValidatorId> validators;
    /// Validators' authority discovery keys for the session in canonical
    /// ordering.
    ///
    /// NOTE: The first `validators.len()` entries will match the corresponding
    /// validators in `validators`, afterwards any remaining authorities can be
    /// found. This is any authorities not participating in parachain consensus
    /// - see
    /// [`max_validators`](https://github.com/paritytech/polkadot/blob/a52dca2be7840b23c19c153cf7e110b1e3e475f8/runtime/parachains/src/configuration.rs#L148)
    std::vector<primitives::AuthorityDiscoveryId> discovery_keys;
    /// The assignment keys for validators.
    ///
    /// NOTE: There might be more authorities in the current session, than
    /// validators participating in parachain consensus. See
    /// [`max_validators`](https://github.com/paritytech/polkadot/blob/a52dca2be7840b23c19c153cf7e110b1e3e475f8/runtime/parachains/src/configuration.rs#L148).
    ///
    /// Therefore:
    /// ```ignore
    ///		assignment_keys.len() == validators.len() && validators.len() <=
    /// discovery_keys.len()
    ///	```
    std::vector<AssignmentId> assignment_keys;
    /// Validators in shuffled ordering - these are the validator groups as
    /// produced by the `Scheduler` module for the session and are typically
    /// referred to by `GroupIndex`.
    std::vector<std::vector<ValidatorIndex>> validator_groups;
    /// The number of availability cores used by the protocol during this
    /// session.
    uint32_t n_cores;
    /// The zeroth delay tranche width.
    uint32_t zeroth_delay_tranche_width;
    /// The number of samples we do of `relay_vrf_modulo`.
    uint32_t relay_vrf_modulo_samples;
    /// The number of delay tranches in total.
    uint32_t n_delay_tranches;
    /// How many slots (BABE / SASSAFRAS) must pass before an assignment is
    /// considered a no-show.
    uint32_t no_show_slots;
    /// The number of validators needed to approve a block.
    uint32_t needed_approvals;
  };

  using InboundDownwardMessage = network::InboundDownwardMessage;
  using InboundHrmpMessage = network::InboundHrmpMessage;

}  // namespace kagome::runtime
#endif  // KAGOME_CORE_RUNTIME_PARACHAIN_HOST_TYPES_HPP
