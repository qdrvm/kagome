/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "dispute_coordinator/chain_scraper.hpp"

#ifndef KAGOME_DISPUTE_SCRAPEDCANDIDATES
#define KAGOME_DISPUTE_SCRAPEDCANDIDATES

namespace kagome::dispute {

  /// Keeps track of scraped candidates. Supports `insert`, `remove_up_to_height` and `contains` operations.
  class ScrapedCandidates final {
//   public:
//    ChainScrapperImpl() {}
//
//    /// Check whether we have seen a candidate included on any chain.
//    bool is_candidate_included(const CandidateHash &candidate_hash) override;
//    /// Check whether the candidate is backed
//    bool is_candidate_backed(const CandidateHash &candidate_hash) override;
//
//   private:
//    /// Main data structure which keeps the candidates we know about. `contains`
//    /// does lookups only here.
//    RefCountedCandidates candidates;
//    /// Keeps track at which block number a candidate was inserted. Used in
//    /// `remove_up_to_height`. Without this tracking we won't be able to remove
//    /// all candidates before block X.
//    BTreeMap<BlockNumber, HashSet<CandidateHash>> candidates_by_block_number,
  };

}

#endif  // KAGOME_DISPUTE_SCRAPEDCANDIDATES
