/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "dispute_coordinator/impl/chain_scrapper_impl.hpp"

namespace kagome::dispute {

  bool ChainScrapperImpl::is_candidate_included(
      const CandidateHash &candidate_hash) {
    return included_candidates_.contains(candidate_hash);
  }

  bool ChainScrapperImpl::is_candidate_backed(
      const CandidateHash &candidate_hash) {
    return backed_candidates_.contains(candidate_hash);
  }

}  // namespace kagome::dispute
