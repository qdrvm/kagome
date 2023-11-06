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

    void add_claims(const libp2p::peer::PeerId &peer,
                    const CandidateClaims &c) {
      if (c.parent_hash_and_id) {
        const auto &pc = *c.parent_hash_and_id;
        auto &sub_claims = parent_claims[pc.first][pc.second];

        bool found = false;
        for (size_t p = 0; p < sub_claims.size(); ++p) {
          if (sub_claims[p].first == c.relay_parent) {
            sub_claims[p].second += 1;
            found = true;
            break;
          }
        }
        if (!found) {
          sub_claims.emplace_back(c.relay_parent, 1);
        }
      }
      claims.emplace_back(peer, c);
    }
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
      return visit_in_place(
          it->second,
          [&](UnconfirmedCandidate &c) {
            c.add_claims(peer,
                         CandidateClaims{
                             .relay_parent = claimed_relay_parent,
                             .group_index = claimed_group_index,
                             .parent_hash_and_id = claimed_parent_hash_and_id,
                         });
            if (claimed_parent_hash_and_id) {
              by_parent[claimed_parent_hash_and_id->first]
                       [claimed_parent_hash_and_id->second]
                           .insert(candidate_hash);
            }
            return true;
          },
          [&](ConfirmedCandidate &c) {
            if (c.receipt.descriptor.relay_parent != claimed_relay_parent) {
              return false;
            }
            if (c.assigned_group != claimed_group_index) {
              return false;
            }
            if (claimed_parent_hash_and_id) {
              const auto &[claimed_parent_hash, claimed_id] =
                  *claimed_parent_hash_and_id;
              if (c.parent_hash != claimed_parent_hash) {
                return false;
              }
              if (c.receipt.descriptor.para_id != claimed_id) {
                return false;
              }
            }
            return true;
          });
    }
  };

}  // namespace kagome::parachain

#endif  // KAGOME_PARACHAIN_CANDIDATES_HPP
