/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "dispute_coordinator/impl/chain_scraper_impl.hpp"

namespace kagome::dispute {

  bool ChainScraperImpl::is_candidate_included(
      const CandidateHash &candidate_hash) {
    return included_candidates_.contains(candidate_hash);
  }

  bool ChainScraperImpl::is_candidate_backed(
      const CandidateHash &candidate_hash) {
    return backed_candidates_.contains(candidate_hash);
  }

  std::vector<primitives::BlockInfo> ChainScraperImpl::get_blocks_including_candidate(
      const CandidateHash &candidate_hash) {
    std::vector<primitives::BlockInfo> res;

    return res;
  }

}  // namespace kagome::dispute
