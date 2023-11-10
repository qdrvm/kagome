/**
 * Copyright Quadrivium Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_PARACHAIN_STATEMENTS_STORE_HPP
#define KAGOME_PARACHAIN_STATEMENTS_STORE_HPP

#include <boost/variant.hpp>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <libp2p/peer/peer_id.hpp>
#include <libp2p/peer/peer_info.hpp>
#include "network/types/collator_messages_vstaging.hpp"
#include "parachain/types.hpp"
#include "parachain/validator/collations.hpp"
#include "primitives/common.hpp"

namespace kagome::parachain {

  struct Groups {
    std::unordered_map<GroupIndex, std::vector<ValidatorIndex>> groups;
    std::unordered_map<ValidatorIndex, GroupIndex> by_validator_index;

    Groups(std::unordered_map<GroupIndex, std::vector<ValidatorIndex>> &&g)
        : groups{std::move(g)} {
      for (const auto &[g, vxs] : groups) {
        for (const auto &v : vxs) {
          by_validator_index[v] = g;
        }
      }
    }

    Groups(const std::vector<std::vector<ValidatorIndex>> &grs) {
      for (GroupIndex g = 0; g < grs.size(); ++g) {
        const auto &group = grs[g];
        groups[g] = group;
        for (const auto &v : group) {
          by_validator_index[v] = g;
        }
      }
    }
  };

  struct ValidatorMeta {
    GroupIndex group;
    size_t within_group_index;
    size_t seconded_count;
  };

  struct GroupStatements {
    scale::BitVec seconded;
    scale::BitVec valid;

    GroupStatements(size_t len) {
      seconded.bits.assign(len, false);
      valid.bits.assign(len, false);
    }

    void note_seconded(size_t within_group_index) {
      BOOST_ASSERT(within_group_index < seconded.bits.size());
      seconded.bits[within_group_index] = true;
    }

    void note_validated(size_t within_group_index) {
      BOOST_ASSERT(within_group_index < valid.bits.size());
      valid.bits[within_group_index] = true;
    }
  };

  struct StoredStatement {
    IndexedAndSigned<network::vstaging::CompactStatement> statement;
    bool known_by_backing;
  };

  struct Fingerprint {
    ValidatorIndex index;
    network::vstaging::CompactStatement statement;

    size_t inner_hash() const {
      size_t r{0ull};
      boost::hash_combine(r, std::hash<ValidatorIndex>()(index));
      visit_in_place(
          statement,
          [&](const network::Empty &) { UNREACHABLE; },
          [&](const auto &v) {
            boost::hash_combine(r, std::hash<CandidateHash>()(v.hash));
          });
      return r;
    }

    bool operator==(const Fingerprint &rhs) const {
      return index == rhs.index && statement == rhs.statement;
    }
  };

  enum StatementOrigin { Local, Remote };

  struct StatementStore {
    std::unordered_map<ValidatorIndex, ValidatorMeta> validator_meta;
    std::unordered_map<GroupIndex,
                       std::unordered_map<CandidateHash, GroupStatements>>
        group_statements;
    std::unordered_map<Fingerprint, StoredStatement, InnerHash<Fingerprint>>
        known_statements;
    log::Logger logger = log::createLogger("StatementStore", "parachain");

    StatementStore(const Groups &groups) {
      for (const auto &[g, vxs] : groups.groups) {
        for (size_t i = 0; i < vxs.size(); ++i) {
          const auto &v = vxs[i];
          validator_meta.emplace(v,
                                 ValidatorMeta{
                                     .group = g,
                                     .within_group_index = i,
                                     .seconded_count = 0,
                                 });
        }
      }
    }

    size_t seconded_count(const ValidatorIndex &validator_index) const {
      auto it = validator_meta.find(validator_index);
      if (it != validator_meta.end()) {
        return it->second.seconded_count;
      }
      return 0;
    }

    void note_known_by_backing(
        ValidatorIndex validator_index,
        const network::vstaging::CompactStatement &statement) {
      auto it = known_statements.find(Fingerprint{
          .index = validator_index,
          .statement = statement,
      });
      if (it != known_statements.end()) {
        auto &stored = it->second;
        stored.known_by_backing = true;
      }
    }

    template <typename F>
    void fresh_statements_for_backing(
        const std::vector<ValidatorIndex> &validators,
        const CandidateHash &candidate_hash,
        F &&cb) const {
      auto call = [&](const Fingerprint &fingerprint) {
        auto it = known_statements.find(fingerprint);
        if (it != known_statements.end()) {
          const StoredStatement &s = it->second;
          if (!s.known_by_backing) {
            std::forward<F>(cb)(s.statement);
          }
        }
      };

      for (const auto &vi : validators) {
        call(Fingerprint{
            .index = vi,
            .statement =
                network::vstaging::SecondedCandidateHash{
                    .hash = candidate_hash,
                },
        });
        call(Fingerprint{
            .index = vi,
            .statement =
                network::vstaging::ValidCandidateHash{
                    .hash = candidate_hash,
                },
        });
      }
    }

    std::optional<bool> insert(
        const Groups &groups,
        const IndexedAndSigned<network::vstaging::CompactStatement> &statement,
        StatementOrigin origin) {
      const auto validator_index = statement.payload.ix;
      auto it = validator_meta.find(validator_index);
      if (it == validator_meta.end()) {
        return std::nullopt;
      }

      auto &vm = it->second;
      const network::vstaging::CompactStatement compact = getPayload(statement);
      Fingerprint fingerprint{
          .index = validator_index,
          .statement = compact,
      };

      auto it_st = known_statements.find(fingerprint);
      if (it_st != known_statements.end()) {
        if (StatementOrigin::Local == origin) {
          it_st->second.known_by_backing = true;
        }
        return false;
      }
      known_statements.emplace(
          fingerprint,
          StoredStatement{
              .statement = statement,
              .known_by_backing = (origin == StatementOrigin::Local),
          });

      const auto &candidate_hash = candidateHash(compact);
      const bool seconded =
          is_type<network::vstaging::SecondedCandidateHash>(compact);
      const auto group_index = vm.group;

      auto it_gr = groups.groups.find(group_index);
      if (it_gr == groups.groups.end()) {
        SL_ERROR(logger,
                 "Groups passed into `insert` differ from those used at store "
                 "creation. (group index={})",
                 group_index);
        return std::nullopt;
      }

      const std::vector<ValidatorIndex> &group = it_gr->second;
      auto [it_s, _] = group_statements[group_index].try_emplace(candidate_hash,
                                                                 group.size());
      if (seconded) {
        vm.seconded_count += 1;
        it_s->second.note_seconded(vm.within_group_index);
      } else {
        it_s->second.note_validated(vm.within_group_index);
      }
      return true;
    }
  };

}  // namespace kagome::parachain

#endif  // KAGOME_PARACHAIN_STATEMENTS_STORE_HPP
