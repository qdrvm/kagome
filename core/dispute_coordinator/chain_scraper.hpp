/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_DISPUTE_CHAINSCRAPER
#define KAGOME_DISPUTE_CHAINSCRAPER

#include "network/types/collator_messages.hpp"
#include "parachain/types.hpp"

namespace kagome::dispute {

  using network::CandidateReceipt;
  using network::DisputeStatement;
  using network::SessionIndex;
  using network::ValidatorIndex;
  using network::Vote;
  using parachain::CandidateHash;
  using parachain::ValidatorSignature;

  /// Chain scraper
  ///
  /// Scrapes unfinalized chain in order to collect information from blocks.
  /// Chain scraping during disputes enables critical spam prevention. It does
  /// so by updating two important criteria determining whether a vote sent
  /// during dispute distribution is potential spam. Namely, whether the
  /// candidate being voted on is backed or included.
  ///
  /// Concretely:
  ///
  /// - Monitors for `CandidateIncluded` events to keep track of candidates that
  /// have been included on chains.
  /// - Monitors for `CandidateBacked` events to keep track of all backed
  /// candidates.
  /// - Calls `FetchOnChainVotes` for each block to gather potentially missed
  /// votes from chain.
  ///
  /// With this information it provides a `CandidateComparator` and as a return
  /// value of `process_active_leaves_update` any scraped votes.
  ///
  /// Scraped candidates are available
  /// `DISPUTE_CANDIDATE_LIFETIME_AFTER_FINALIZATION` more blocks after
  /// finalization as a precaution not to prune them prematurely.
  class ChainScraper {
   public:
    virtual ~ChainScraper() = default;

    /// Check whether we have seen a candidate included on any chain.
    virtual bool is_candidate_included(const CandidateHash &candidate_hash) = 0;

    /// Check whether the candidate is backed
    virtual bool is_candidate_backed(const CandidateHash &candidate_hash) = 0;
  };

}  // namespace kagome::dispute

#endif  // KAGOME_DISPUTE_CHAINSCRAPER
