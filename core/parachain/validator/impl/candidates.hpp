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

    bool check(const RelayHash &rp,
               GroupIndex gi,
               const Hash &ph,
               ParachainId pi) const {
      bool ch = true;
      if (parent_hash_and_id) {
        ch =
            parent_hash_and_id->first == ph && parent_hash_and_id->second == pi;
      }

      return relay_parent == rp && group_index == gi && ch;
    }
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

    void extend_hypotheticals(
        const CandidateHash &candidate_hash,
        std::vector<HypotheticalCandidate> &v,
        const std::optional<std::pair<std::reference_wrapper<const Hash>,
                                      ParachainId>> &required_parent) const {
      auto extend_hypotheticals_inner =
          [&](const Hash &parent_head_hash,
              ParachainId para_id,
              const std::vector<std::pair<Hash, size_t>>
                  &possible_relay_parents) {
            for (const auto &[relay_parent, _] : possible_relay_parents) {
              v.emplace_back(HypotheticalCandidateIncomplete{
                  .candidate_hash = candidate_hash,
                  .candidate_para = para_id,
                  .parent_head_data_hash = parent_head_hash,
                  .candidate_relay_parent = relay_parent,
              });
            }
          };

      if (required_parent) {
        if (auto h_it = parent_claims.find(required_parent->first.get());
            h_it != parent_claims.end()) {
          if (auto p_it = h_it->second.find(required_parent->second);
              p_it != h_it->second.end()) {
            extend_hypotheticals_inner(h_it->first, p_it->first, p_it->second);
          }
        }
      } else {
        for (const auto &[h, m] : parent_claims) {
          for (const auto &[p, d] : m) {
            extend_hypotheticals_inner(h, p, d);
          }
        }
      }
    }

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

    HypotheticalCandidate to_hypothetical(
        const CandidateHash &candidate_hash) const {
      return HypotheticalCandidateComplete{
          .candidate_hash = candidate_hash,
          .receipt = receipt,
          .persisted_validation_data = persisted_validation_data,
      };
    }

    bool is_importable(std::optional<std::reference_wrapper<const Hash>>
                           under_active_leaf) const {
      if (!under_active_leaf) {
        return !importable_under.empty();
      }
      return importable_under.find(under_active_leaf->get())
          != importable_under.end();
    }
  };

  using CandidateState =
      boost::variant<UnconfirmedCandidate, ConfirmedCandidate>;

  struct Candidates {
    std::unordered_map<CandidateHash, CandidateState> candidates;
    std::unordered_map<
        Hash,
        std::unordered_map<ParachainId, std::unordered_set<CandidateHash>>>
        by_parent;

    std::vector<HypotheticalCandidate> frontier_hypotheticals(
        const std::optional<std::pair<std::reference_wrapper<const Hash>,
                                      ParachainId>> &parent) const {
      std::vector<HypotheticalCandidate> v;
      auto extend_hypotheticals =
          [&](const CandidateHash &ch,
              const CandidateState &cs,
              const std::optional<
                  std::pair<std::reference_wrapper<const Hash>, ParachainId>>
                  &maybe_required_parent) {
            visit_in_place(
                cs,
                [&](const UnconfirmedCandidate &u) {
                  u.extend_hypotheticals(ch, v, maybe_required_parent);
                },
                [&](const ConfirmedCandidate &c) {
                  v.emplace_back(c.to_hypothetical(ch));
                });
          };

      if (parent) {
        if (auto h_it = by_parent.find(parent->first);
            h_it != by_parent.end()) {
          if (auto p_it = h_it->second.find(parent->second);
              p_it != h_it->second.end()) {
            for (const auto &ch : p_it->second) {
              if (auto it = candidates.find(ch); it != candidates.end()) {
                extend_hypotheticals(ch, it->second, parent);
              }
            }
          }
        }
      } else {
        for (const auto &[ch, cs] : candidates) {
          extend_hypotheticals(ch, cs, std::nullopt);
        }
      }

      return v;
    }

    bool is_confirmed(const CandidateHash &candidate_hash) const {
      auto it = candidates.find(candidate_hash);
      if (it != candidates.end()) {
        return is_type<const ConfirmedCandidate>(it->second);
      }
      return false;
    }

    bool is_importable(const CandidateHash &candidate_hash) const {
      auto opt_confirmed = get_confirmed(candidate_hash);
      if (!opt_confirmed) {
        return false;
      }
      return opt_confirmed->get().is_importable(std::nullopt);
    }

    std::optional<std::reference_wrapper<const ConfirmedCandidate>>
    get_confirmed(const CandidateHash &candidate_hash) const {
      auto it = candidates.find(candidate_hash);
      if (it != candidates.end()) {
        return if_type<const ConfirmedCandidate>(it->second);
      }
      return std::nullopt;
    }

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

    std::optional<PostConfirmation> confirm_candidate(
        const CandidateHash &candidate_hash,
        const network::CommittedCandidateReceipt &candidate_receipt,
        const runtime::PersistedValidationData &persisted_validation_data,
        GroupIndex assigned_group,
        const std::shared_ptr<crypto::Hasher> &hasher) {
      const auto parent_hash =
          hasher->blake2b_256(persisted_validation_data.parent_head);
      const auto &relay_parent = candidate_receipt.descriptor.relay_parent;
      const auto para_id = candidate_receipt.descriptor.para_id;

      std::optional<CandidateState> prev_state;
      if (auto it = candidates.find(candidate_hash); it != candidates.end()) {
        prev_state = it->second;
      }

      ConfirmedCandidate &new_confirmed =
          [&]() -> std::reference_wrapper<ConfirmedCandidate> {
        auto [it, _] = candidates.insert_or_assign(
            candidate_hash,
            ConfirmedCandidate{
                .receipt = candidate_receipt,
                .persisted_validation_data = persisted_validation_data,
                .assigned_group = assigned_group,
                .parent_hash = parent_hash,
                .importable_under = {},
            });
        auto n{boost::get<ConfirmedCandidate>(&it->second)};
        return {*n};
      }()
                       .get();
      by_parent[parent_hash][para_id].insert(candidate_hash);

      if (!prev_state) {
        return PostConfirmation{
            .hypothetical = new_confirmed.to_hypothetical(candidate_hash),
            .reckoning = {},
        };
      }

      if (auto ps = if_type<ConfirmedCandidate>(*prev_state)) {
        return std::nullopt;
      }

      auto u = if_type<UnconfirmedCandidate>(*prev_state);
      PostConfirmationReckoning reckoning{};

      for (const UnconfiredImportablePair &d :
           u->get().unconfirmed_importable_under) {
        const auto &leaf_hash = d.hash;
        const auto &x = d.ui;

        if (x.relay_parent == relay_parent && x.parent_hash == parent_hash
            && x.para_id == para_id) {
          new_confirmed.importable_under.insert(leaf_hash);
        }
      }

      for (const auto &[peer, claims] : u->get().claims) {
        if (claims.parent_hash_and_id) {
          const auto &[claimed_parent_hash, claimed_id] =
              *claims.parent_hash_and_id;
          if (claimed_parent_hash != parent_hash || claimed_id != para_id) {
            if (auto it_1 = by_parent.find(claimed_parent_hash);
                it_1 != by_parent.end()) {
              if (auto it_2 = it_1->second.find(claimed_id);
                  it_2 != it_1->second.end()) {
                it_2->second.erase(candidate_hash);
                if (it_2->second.empty()) {
                  it_1->second.erase(it_2);
                  if (it_1->second.empty()) {
                    by_parent.erase(it_1);
                  }
                }
              }
            }
          }
        }

        if (claims.check(relay_parent, assigned_group, parent_hash, para_id)) {
          reckoning.correct.insert(peer);
        } else {
          reckoning.incorrect.insert(peer);
        }
      }

      return PostConfirmation{
          .hypothetical = new_confirmed.to_hypothetical(candidate_hash),
          .reckoning = reckoning,
      };
    }
  };

}  // namespace kagome::parachain

#endif  // KAGOME_PARACHAIN_CANDIDATES_HPP
