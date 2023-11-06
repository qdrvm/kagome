/**
 * Copyright Quadrivium Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_PARACHAIN_CANDIDATES_HPP
#define KAGOME_PARACHAIN_CANDIDATES_HPP

#include <boost/variant.hpp>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <libp2p/peer/peer_id.hpp>
#include <libp2p/peer/peer_info.hpp>
#include "parachain/validator/collations.hpp"
#include "primitives/common.hpp"

namespace kagome::parachain {

  struct PostConfirmationReckoning {
    std::unordered_set<libp2p::peer::PeerId> correct;
    std::unordered_set<libp2p::peer::PeerId> incorrect;
  };

  struct PostConfirmation {
    HypotheticalCandidate hypothetical;
    PostConfirmationReckoning reckoning;
  };

  struct CandidateClaims {
    RelayHash relay_parent;
    GroupIndex group_index;
    std::optional<std::pair<Hash, ParachainId>> parent_hash_and_id;
  };

  struct UnconfirmedImportable {
    RelayHash relay_parent;
    Hash parent_hash;
    ParachainId para_id;

    bool operator==(const UnconfirmedImportable &rhs) const {
      return para_id == rhs.para_id && relay_parent == rhs.relay_parent
          && parent_hash == rhs.parent_hash;
    }
  };

  struct UnconfiredImportablePair {
    Hash hash;
    UnconfirmedImportable ui;

    size_t inner_hash() const {
      size_t r{0ull};
      boost::hash_combine(r, std::hash<Hash>()(hash));
      boost::hash_combine(r, std::hash<RelayHash>()(ui.relay_parent));
      boost::hash_combine(r, std::hash<Hash>()(ui.parent_hash));
      boost::hash_combine(r, std::hash<ParachainId>()(ui.para_id));
      return r;
    }

    bool operator==(const UnconfiredImportablePair &rhs) const {
      return hash == rhs.hash && ui == rhs.ui;
    }
  };

  struct UnconfirmedCandidate {
    std::vector<std::pair<libp2p::peer::PeerId, CandidateClaims>> claims;
    std::unordered_map<
        Hash,
        std::unordered_map<ParachainId, std::vector<std::pair<Hash, size_t>>>>
        parent_claims;
    std::unordered_set<UnconfiredImportablePair,
                       InnerHash<UnconfiredImportablePair>>
        unconfirmed_importable_under;
  };
  struct ConfirmedCandidate {
    network::CommittedCandidateReceipt receipt;
    runtime::PersistedValidationData persisted_validation_data;
    GroupIndex assigned_group;
    RelayHash parent_hash;
    std::unordered_set<Hash> importable_under;
  };
  using CandidateState =
      boost::variant<UnconfirmedCandidate, ConfirmedCandidate>;

  struct Candidates {
    std::unordered_map<CandidateHash, CandidateState> candidates;
    std::unordered_map<
        Hash,
        std::unordered_map<ParachainId, std::unordered_set<CandidateHash>>>
        by_parent;

    bool insert_unconfirmed(const libp2p::peer::PeerId &peer,
                            const CandidateHash &candidate_hash,
                            const Hash &claimed_relay_parent,
                            GroupIndex claimed_group_index,
                            const std::optional<std::pair<Hash, ParachainId>>
                                &claimed_parent_hash_and_id) {
      const auto &[it, inserted] =
          candidates.emplace(candidate_hash, UnconfirmedCandidate{});
      visit_in_place(
          it->second,
          [&](UnconfirmedCandidate &c) {

          },
          [&](ConfirmedCandidate &c) {

          });

      /// TODO(iceseer): do
      return false;
    }
  };

}  // namespace kagome::parachain

#endif  // KAGOME_PARACHAIN_CANDIDATES_HPP
