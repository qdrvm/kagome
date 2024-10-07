/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "parachain/validator/prospective_parachains/common.hpp"
#include "parachain/validator/prospective_parachains/fragment_node.hpp"

namespace kagome::parachain::fragment {

  struct BackedChain {
    // Holds the candidate chain.
    Vec<FragmentNode> chain;
    // Index from head data hash to the candidate hash with that head data as a
    // parent. Only contains the candidates present in the `chain`.
    HashMap<Hash, CandidateHash> by_parent_head;
    // Index from head data hash to the candidate hash outputting that head
    // data. Only contains the candidates present in the `chain`.
    HashMap<Hash, CandidateHash> by_output_head;
    // A set of the candidate hashes in the `chain`.
    HashSet<CandidateHash> candidates;

    void push(FragmentNode candidate);
    bool contains(const CandidateHash &hash) const;
    Vec<FragmentNode> clear();
    Vec<FragmentNode> revert_to_parent_hash(const Hash &parent_head_data_hash);
  };

}  // namespace kagome::parachain::fragment
