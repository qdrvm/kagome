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
#include "parachain/validator/collations.hpp"
#include "primitives/common.hpp"
#include "parachain/types.hpp"
#include "network/types/collator_messages_vstaging.hpp"

namespace kagome::parachain {

struct Groups {
	std::unordered_map<GroupIndex, std::vector<ValidatorIndex>> groups;
	std::unordered_map<ValidatorIndex, GroupIndex> by_validator_index;

    Groups(std::unordered_map<GroupIndex, std::vector<ValidatorIndex>> &&g) : groups{std::move(g)} {
        for (const auto &[g, vxs] : groups) {
            for (const auto &v : vxs) {
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
      visit_in_place(statement,
        [&](const network::Empty &) {
            UNREACHABLE;
        },
        [&](const auto &v) {
            boost::hash_combine(r, std::hash<CandidateHash>()(v.hash));
        });
      return r;
    }

    bool operator==(const Fingerprint &rhs) const {
      return index == rhs.index && statement == rhs.statement;
    }
};

struct StatementStore {
	std::unordered_map<ValidatorIndex, ValidatorMeta> validator_meta;
	std::unordered_map<GroupIndex, std::unordered_map<CandidateHash, GroupStatements>> group_statements;
	std::unordered_map<Fingerprint, StoredStatement, InnerHash<Fingerprint>> known_statements;

    StatementStore(const Groups &groups) {
        for (const auto &[g, vxs] : groups.groups) {
            for (size_t i = 0; i < vxs.size(); ++i) {
                const auto &v = vxs[i];
                validator_meta.emplace(v, ValidatorMeta {
						.group = g,
						.within_group_index = i,
						.seconded_count = 0,
					});
            }
        }
    }
};


}  // namespace kagome::parachain

#endif  // KAGOME_PARACHAIN_STATEMENTS_STORE_HPP
