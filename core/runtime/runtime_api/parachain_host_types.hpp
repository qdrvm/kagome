/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_PARACHAIN_HOST_TYPES_HPP
#define KAGOME_CORE_RUNTIME_PARACHAIN_HOST_TYPES_HPP

#include <scale/bitvec.hpp>

#include "common/blob.hpp"
#include "common/unused.hpp"
#include "network/types/collator_messages.hpp"
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

    bool operator==(const OccupiedCore &rhs) const {
      return next_up_on_available == rhs.next_up_on_available
         and occupied_since == rhs.occupied_since
         and time_out_at == rhs.time_out_at
         and next_up_on_time_out == rhs.next_up_on_time_out
         and availability == rhs.availability
         and group_responsible == rhs.group_responsible
         and candidate_hash == rhs.candidate_hash
         and candidate_descriptor == rhs.candidate_descriptor;
    }
  };

  struct GroupDescriptor {
    SCALE_TIE(3);

    /// The block number where the session started.
    BlockNumber session_start_block;

    /// How often groups rotate. 0 means never.
    GroupRotatePeriod group_rotation_frequency;

    /// The current block number.
    BlockNumber now_block_num;

    /// Returns the index of the group needed to validate the core at the given
    /// index, assuming the given number of cores.
    GroupIndex groupForCore(CoreIndex core_index, size_t cores) {
      if (group_rotation_frequency == 0) {
        return core_index;
      }
      if (cores == 0ull) {
        return 0;
      }

      const auto cores_normalized =
          std::min(cores, size_t(std::numeric_limits<CoreIndex>::max()));
      const auto blocks_since_start =
          math::sat_sub_unsigned(now_block_num, session_start_block);
      const auto rotations = blocks_since_start / group_rotation_frequency;

      /// g = c + r mod cores_normalized
      return GroupIndex((size_t(core_index) + size_t(rotations))
                        % cores_normalized);
    }
  };

  struct ValidatorGroup {
    SCALE_TIE(1);
    std::vector<kagome::parachain::ValidatorIndex> validators;

    bool contains(kagome::parachain::ValidatorIndex validator_ix) const {
      return std::find(validators.begin(), validators.end(), validator_ix)
          != validators.end();
    }
  };

  using FreeCore = Empty;

  using ValidatorGroupsAndDescriptor =
      std::tuple<std::vector<ValidatorGroup>, GroupDescriptor>;
  using CoreState = std::variant<OccupiedCore,   // 0
                                 ScheduledCore,  // 1
                                 FreeCore>;      // 2
  enum class OccupiedCoreAssumption : uint8_t {
    Included,  // 0
    TimedOut,  // 1
    Unused     // 2
  };
  struct PersistedValidationData {
    SCALE_TIE(4);

    /// The parent head-data.
    HeadData parent_head;
    /// The relay-chain block number this is in the context of.
    BlockNumber relay_parent_number;
    /// The relay-chain block storage root this is in the context of.
    Hash relay_parent_storage_root;
    /// The maximum legal size of a POV block, in bytes.
    uint32_t max_pov_size;
  };

  struct AvailableData {
    SCALE_TIE(2);

    /// The Proof-of-Validation of the candidate.
    network::ParachainBlock pov;
    /// The persisted validation data needed for secondary checks.
    runtime::PersistedValidationData validation_data;
  };

  using OutboundHrmpMessage = network::OutboundHorizontal;
  using CandidateCommitments = network::CandidateCommitments;
  using CommittedCandidateReceipt = network::CommittedCandidateReceipt;
  using CandidateReceipt = network::CandidateReceipt;

  struct Candidate {
    CandidateReceipt candidate_receipt;
    HeadData head_data;
    CoreIndex core_index;

    bool operator==(const Candidate &rhs) const {
      return candidate_receipt == rhs.candidate_receipt
         and head_data == rhs.head_data  //
         and core_index == rhs.core_index;
    }
  };

  struct CandidateBacked : public Candidate {
    GroupIndex group_index{};

    bool operator==(const CandidateBacked &rhs) const {
      return (const Candidate &)(*this) == (const Candidate &)rhs
         and group_index == rhs.group_index;
    }
  };

  struct CandidateIncluded
      : public Candidate,
        public boost::equality_comparable<CandidateIncluded> {
    GroupIndex group_index{};

    bool operator==(const CandidateIncluded &rhs) const {
      return (const Candidate &)(*this) == (const Candidate &)rhs
         and group_index == rhs.group_index;
    }
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
    SCALE_TIE(13);

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

    bool operator==(const SessionInfo &rhs) const {
      return active_validator_indices == rhs.active_validator_indices
         and random_seed == rhs.random_seed
         and dispute_period == rhs.dispute_period
         and validators == rhs.validators
         and discovery_keys == rhs.discovery_keys
         and assignment_keys == rhs.assignment_keys
         and validator_groups == rhs.validator_groups and n_cores == rhs.n_cores
         and zeroth_delay_tranche_width == rhs.zeroth_delay_tranche_width
         and relay_vrf_modulo_samples == rhs.relay_vrf_modulo_samples
         and n_delay_tranches == rhs.n_delay_tranches
         and no_show_slots == rhs.no_show_slots
         and needed_approvals == rhs.needed_approvals;
    }
    bool operator!=(const SessionInfo &rhs) const {
      return !operator==(rhs);
    }
  };

  using InboundDownwardMessage = network::InboundDownwardMessage;
  using InboundHrmpMessage = network::InboundHrmpMessage;

  enum class PvfPrepTimeoutKind {
    /// For prechecking requests, the time period after which the preparation
    /// worker is considered
    /// unresponsive and will be killed.
    Precheck,

    /// For execution and heads-up requests, the time period after which the
    /// preparation worker is
    /// considered unresponsive and will be killed. More lenient than the
    /// timeout for prechecking
    /// to prevent honest validators from timing out on valid PVFs.
    Lenient,
  };

  /// Type discriminator for PVF execution timeouts
  enum class PvfExecTimeoutKind {
    /// The amount of time to spend on execution during backing.
    Backing,

    /// The amount of time to spend on execution during approval or disputes.
    ///
    /// This should be much longer than the backing execution timeout to ensure
    /// that in the
    /// absence of extremely large disparities between hardware, blocks that
    /// pass backing are
    /// considered executable by approval checkers or dispute participants.
    Approval,
  };

  /// Maximum number of memory pages (64KiB bytes per page) the executor can
  /// allocate.
  struct MaxMemoryPages {
    SCALE_TIE(1)
    uint32_t limit;
  };

  /// Wasm logical stack size limit (max. number of Wasm values on stack)
  struct StackLogicalMax {
    SCALE_TIE(1)
    uint32_t max_values_num;
  };
  /// Executor machine stack size limit, in bytes
  struct StackNativeMax {
    SCALE_TIE(1)
    uint32_t max_bytes_num;
  };
  /// Max. amount of memory the preparation worker is allowed to use during
  /// pre-checking, in bytes
  struct PrecheckingMaxMemory {
    SCALE_TIE(1)
    uint64_t max_bytes_num;
  };
  /// PVF preparation timeouts, millisec
  struct PvfPrepTimeout {
    SCALE_TIE(2)
    PvfPrepTimeoutKind kind;
    uint64_t msec;
  };
  /// PVF execution timeouts, millisec
  struct PvfExecTimeout {
    SCALE_TIE(2)
    PvfExecTimeoutKind kind;
    uint64_t msec;
  };

  /// Enables WASM bulk memory proposal
  using WasmExtBulkMemory = Unused<1>;

  using ExecutorParam = boost::variant<Unused<0>,
                                       MaxMemoryPages,
                                       StackLogicalMax,
                                       StackNativeMax,
                                       PrecheckingMaxMemory,
                                       PvfPrepTimeout,
                                       PvfExecTimeout,
                                       Unused<7>>;  // WasmExtBulkMemory

}  // namespace kagome::runtime
#endif  // KAGOME_CORE_RUNTIME_PARACHAIN_HOST_TYPES_HPP
