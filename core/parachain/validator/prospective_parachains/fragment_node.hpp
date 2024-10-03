/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "parachain/validator/prospective_parachains/common.hpp"
#include "parachain/validator/prospective_parachains/fragment.hpp"

namespace kagome::parachain::fragment {

  struct FragmentNode {
    Fragment fragment;
    CandidateHash candidate_hash;
    ConstraintModifications cumulative_modifications;
    Hash parent_head_data_hash;
    Hash output_head_data_hash;

    const Hash &relay_parent() const {
      return fragment.get_relay_parent().hash;
    }

    CandidateEntry into_candidate_entry() const {
      return CandidateEntry{
          .candidate_hash = this->candidate_hash,
          .parent_head_data_hash = this->parent_head_data_hash,
          .output_head_data_hash = this->output_head_data_hash,
          .relay_parent = this->relay_parent(),
          .candidate = this->fragment.get_candidate_clone(),
          .state = CandidateState::Backed,
      };
    }
  };

}  // namespace kagome::parachain::fragment
