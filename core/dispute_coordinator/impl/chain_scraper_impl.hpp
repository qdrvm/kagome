/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "dispute_coordinator/chain_scraper.hpp"

#ifndef KAGOME_DISPUTE_CHAINSCRAPERIMPL
#define KAGOME_DISPUTE_CHAINSCRAPERIMPL

namespace kagome::dispute {

  class ChainScraperImpl final : public ChainScraper {
   public:
    ChainScraperImpl() {}

    /// Check whether we have seen a candidate included on any chain.
    bool is_candidate_included(const CandidateHash &candidate_hash) override;
    /// Check whether the candidate is backed
    bool is_candidate_backed(const CandidateHash &candidate_hash) override;

   private:
    /// All candidates we have seen included, which not yet have been finalized.
    candidates::ScrapedCandidates included_candidates_;

    /// All candidates we have seen backed
    candidates::ScrapedCandidates backed_candidates_;

    /// Latest relay blocks observed by the provider.
    ///
    /// We assume that ancestors of cached blocks are already processed, i.e. we
    /// have saved corresponding included candidates.
    LruCache<Hash, ()> last_observed_blocks_;

    /// Maps included candidate hashes to one or more relay block heights and
    /// hashes. These correspond to all the relay blocks which marked a candidate as included, and are needed to apply reversions in case a dispute is concluded against the candidate.
    Inclusions inclusions_;
  };

}

#endif  // KAGOME_DISPUTE_CHAINSCRAPPERIMPL
