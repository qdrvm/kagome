/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <boost/variant.hpp>
#include <map>
#include <optional>
#include <unordered_map>

#include "crypto/type_hasher.hpp"
#include "log/logger.hpp"
#include "network/types/collator_messages.hpp"
#include "parachain/types.hpp"
#include "runtime/runtime_api/parachain_host_types.hpp"

namespace kagome::parachain {

  struct ProspectiveParachainsMode {
    /// The maximum number of para blocks between the para head in a relay
    /// parent and a new candidate. Restricts nodes from building arbitrary
    /// long chains and spamming other validators.
    size_t max_candidate_depth;

    /// How many ancestors of a relay parent are allowed to build candidates
    /// on top of.
    size_t allowed_ancestry_len;
  };
  using ProspectiveParachainsModeOpt = std::optional<ProspectiveParachainsMode>;

  struct ActiveLeafState {
    ProspectiveParachainsModeOpt prospective_parachains_mode;
    /// The candidates seconded at various depths under this active
    /// leaf with respect to parachain id. A candidate can only be
    /// seconded when its hypothetical frontier under every active leaf
    /// has an empty entry in this map.
    ///
    /// When prospective parachains are disabled, the only depth
    /// which is allowed is 0.
    std::unordered_map<ParachainId, std::map<size_t, CandidateHash>>
        seconded_at_depth;
  };

  /// The status of the collations.
  enum struct CollationStatus {
    /// We are waiting for a collation to be advertised to us.
    Waiting,
    /// We are currently fetching a collation.
    Fetching,
    /// We are waiting that a collation is being validated.
    WaitingOnValidation,
    /// We have seconded a collation.
    Seconded,
  };

  using ProspectiveCandidate = kagome::network::ProspectiveCandidate;
  using PendingCollation = kagome::network::PendingCollation;

  struct PendingCollationHash {
    size_t operator()(const PendingCollation &val) const {
      size_t r{0ull};
      boost::hash_combine(r, std::hash<RelayHash>()(val.relay_parent));
      boost::hash_combine(r, std::hash<network::ParachainId>()(val.para_id));
      if (val.prospective_candidate) {
        boost::hash_combine(r,
                            std::hash<CandidateHash>()(
                                val.prospective_candidate->candidate_hash));
      }
      return r;
    }
  };

  struct PendingCollationEq {
    bool operator()(const PendingCollation &__x,
                    const PendingCollation &__y) const {
      return __x.relay_parent == __y.relay_parent && __x.para_id == __y.para_id
          && __x.prospective_candidate == __y.prospective_candidate;
    }
  };

  struct Collations {
    /// How many collations have been seconded.
    size_t seconded_count{0ull};
    /// What is the current status in regards to a collation for this relay
    /// parent?
    CollationStatus status{CollationStatus::Waiting};
    /// Collation that were advertised to us, but we did not yet fetch.
    std::deque<std::pair<PendingCollation, CollatorId>> waiting_queue{};
    /// Collator we're fetching from, optionally which candidate was requested.
    ///
    /// This is the currently last started fetch, which did not exceed
    /// `MAX_UNSHARED_DOWNLOAD_TIME` yet.
    std::optional<std::pair<CollatorId, std::optional<CandidateHash>>>
        fetching_from{};

    bool hasSecondedSpace(
        const ProspectiveParachainsModeOpt &relay_parent_mode) {
      const auto seconded_limit =
          relay_parent_mode ? relay_parent_mode->max_candidate_depth + 1 : 1;
      return seconded_count < seconded_limit;
    }

    /// Note a seconded collation for a given para.
    void note_seconded() {
      seconded_count += 1;
    }

    void back_to_waiting(
        const ProspectiveParachainsModeOpt &relay_parent_mode) {
      if (status != CollationStatus::Seconded || relay_parent_mode) {
        status = CollationStatus::Waiting;
      }
    }

    /// Returns the next collation to fetch from the `waiting_queue`.
    ///
    /// This will reset the status back to `Waiting` using
    /// [`CollationStatus::back_to_waiting`].
    ///
    /// Returns `Some(_)` if there is any collation to fetch, the `status` is
    /// not `Seconded` and the passed in `finished_one` is the currently
    /// `waiting_collation`.
    std::optional<std::pair<PendingCollation, CollatorId>>
    get_next_collation_to_fetch(
        const std::pair<CollatorId, std::optional<CandidateHash>> &finished_one,
        const ProspectiveParachainsModeOpt &relay_parent_mode,
        log::Logger logger) {
      if (fetching_from) {
        const auto &[collator_id, maybe_candidate_hash] = *fetching_from;
        if (collator_id != finished_one.first
            && (!maybe_candidate_hash
                || *maybe_candidate_hash != finished_one.second)) {
          SL_TRACE(
              logger,
              "Not proceeding to the next collation - has already been done.");
          return std::nullopt;
        }
      }

      back_to_waiting(relay_parent_mode);
      switch (status) {
        case CollationStatus::Seconded:
          return std::nullopt;
        case CollationStatus::Waiting: {
          if (!is_seconded_limit_reached(relay_parent_mode)) {
            return std::nullopt;
          }
          if (waiting_queue.empty()) {
            return std::nullopt;
          }
          std::pair<PendingCollation, CollatorId> v{
              std::move(waiting_queue.front())};
          waiting_queue.pop_front();
          return v;
        } break;
        case CollationStatus::WaitingOnValidation:
        case CollationStatus::Fetching: {
          UNREACHABLE;
        } break;
      }
      return std::nullopt;
    }

    bool is_seconded_limit_reached(
        const ProspectiveParachainsModeOpt &relay_parent_mode) const {
      if (relay_parent_mode) {
        return seconded_count < (relay_parent_mode->max_candidate_depth + 1);
      }
      return seconded_count < 1;
    }
  };

  struct HypotheticalCandidateComplete {
    /// The hash of the candidate.
    CandidateHash candidate_hash;
    /// The receipt of the candidate.
    network::CommittedCandidateReceipt receipt;
    /// The persisted validation data of the candidate.
    runtime::PersistedValidationData persisted_validation_data;

    bool operator==(const HypotheticalCandidateComplete &r) const = default;
  };

  struct HypotheticalCandidateIncomplete {
    /// The claimed hash of the candidate.
    CandidateHash candidate_hash;
    /// The claimed para-ID of the candidate.
    ParachainId candidate_para;
    /// The claimed head-data hash of the candidate.
    Hash parent_head_data_hash;
    /// The claimed relay parent of the candidate.
    Hash candidate_relay_parent;

    bool operator==(const HypotheticalCandidateIncomplete &r) const = default;
  };

  struct BlockedAdvertisement {
    /// Peer that advertised the collation.
    libp2p::peer::PeerId peer_id;
    /// Collator id.
    CollatorId collator_id;
    /// The relay-parent of the candidate.
    Hash candidate_relay_parent;
    /// Hash of the candidate.
    CandidateHash candidate_hash;
  };

  /// A hypothetical candidate to be evaluated for frontier membership
  /// in the prospective parachains subsystem.
  ///
  /// Hypothetical candidates are either complete or incomplete.
  /// Complete candidates have already had their (potentially heavy)
  /// candidate receipt fetched, while incomplete candidates are simply
  /// claims about properties that a fetched candidate would have.
  ///
  /// Complete candidates can be evaluated more strictly than incomplete
  /// candidates.
  using HypotheticalCandidate = boost::variant<HypotheticalCandidateComplete,
                                               HypotheticalCandidateIncomplete>;

  inline std::reference_wrapper<const ParachainId> candidatePara(
      const HypotheticalCandidate &hc) {
    return visit_in_place(
        hc,
        [](const HypotheticalCandidateComplete &val)
            -> std::reference_wrapper<const ParachainId> {
          return val.receipt.descriptor.para_id;
        },
        [](const HypotheticalCandidateIncomplete &val)
            -> std::reference_wrapper<const ParachainId> {
          return val.candidate_para;
        });
  }

  inline Hash parentHeadDataHash(const crypto::Hasher &hasher,
                                 const HypotheticalCandidate &hc) {
    return visit_in_place(
        hc,
        [&](const HypotheticalCandidateComplete &val) {
          return hasher.blake2b_256(val.persisted_validation_data.parent_head);
        },
        [](const HypotheticalCandidateIncomplete &val) {
          return val.parent_head_data_hash;
        });
  }

  inline std::reference_wrapper<const RelayHash> relayParent(
      const HypotheticalCandidate &hc) {
    return visit_in_place(
        hc,
        [](const HypotheticalCandidateComplete &val)
            -> std::reference_wrapper<const RelayHash> {
          return val.receipt.descriptor.relay_parent;
        },
        [](const HypotheticalCandidateIncomplete &val)
            -> std::reference_wrapper<const RelayHash> {
          return val.candidate_relay_parent;
        });
  }

  inline std::reference_wrapper<const CandidateHash> candidateHash(
      const HypotheticalCandidate &hc) {
    return visit_in_place(
        hc, [](const auto &val) -> std::reference_wrapper<const CandidateHash> {
          return val.candidate_hash;
        });
  }

}  // namespace kagome::parachain
