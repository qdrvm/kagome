/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "parachain/validator/prospective_parachains/backed_chain.hpp"
#include "utils/stringify.hpp"

#define COMPONENT BackedChain
#define COMPONENT_NAME STRINGIFY(COMPONENT)

namespace kagome::parachain::fragment {

  void BackedChain::push(FragmentNode candidate) {
    candidates.emplace(candidate.candidate_hash);
    by_parent_head.insert_or_assign(candidate.parent_head_data_hash,
                                    candidate.candidate_hash);
    by_output_head.insert_or_assign(candidate.output_head_data_hash,
                                    candidate.candidate_hash);
    chain.emplace_back(candidate);
  }

  Vec<FragmentNode> BackedChain::clear() {
    by_parent_head.clear();
    by_output_head.clear();
    candidates.clear();
    return std::move(chain);
  }

  bool BackedChain::contains(const CandidateHash &hash) const {
    return candidates.contains(hash);
  }

  Vec<FragmentNode> BackedChain::revert_to_parent_hash(
      const Hash &parent_head_data_hash) {
    Option<size_t> found_index;
    for (size_t index = 0; index < chain.size(); ++index) {
      const auto &node = chain[0];
      if (found_index) {
        by_parent_head.erase(node.parent_head_data_hash);
        by_output_head.erase(node.output_head_data_hash);
        candidates.erase(node.candidate_hash);
      } else if (node.output_head_data_hash == parent_head_data_hash) {
        found_index = index;
      }
    }
    if (found_index) {
      auto it_from = chain.begin();
      std::advance(it_from,
                   std::min(*found_index + size_t(1ull), chain.size()));

      Vec<FragmentNode> removed{std::move_iterator(it_from),
                                std::move_iterator(chain.end())};
      chain.erase(it_from, chain.end());
      return removed;
    }
    return {};
  }

}  // namespace kagome::parachain::fragment
