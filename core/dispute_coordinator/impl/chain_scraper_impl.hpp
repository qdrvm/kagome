/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "dispute_coordinator/chain_scraper.hpp"

#ifndef KAGOME_DISPUTE_CHAINSCRAPERIMPL
#define KAGOME_DISPUTE_CHAINSCRAPERIMPL

#include "blockchain/block_tree.hpp"
#include "dispute_coordinator/types.hpp"

namespace kagome::runtime {
  class ParachainHost;
}

namespace kagome::dispute {

  /// Keeps track of scraped candidates. Supports `insert`,
  /// `remove_up_to_height` and `contains` operations.
  struct ScrapedCandidates {
    /// Main data structure which keeps the candidates we know about. `contains`
    /// does lookups only here.
    std::unordered_map<CandidateHash, size_t> candidates_{};

    /// Keeps track at which block number a candidate was inserted. Used in
    /// `remove_up_to_height`. Without this tracking we won't be able to remove
    /// all candidates before block X.
    std::map<primitives::BlockNumber, std::unordered_set<CandidateHash>>
        candidates_by_block_number_;

    bool contains(const CandidateHash &candidate_hash) {
      return candidates_.count(candidate_hash);
    }

    // Removes all candidates up to a given height.
    // The candidates at the block height are NOT removed.
    std::unordered_set<CandidateHash> remove_up_to_height(
        primitives::BlockNumber height) {
      std::unordered_set<CandidateHash> candidates_modified;
      auto end = candidates_by_block_number_.upper_bound(height);
      for (auto it = candidates_by_block_number_.begin(); it != end;) {
        auto cit = it++;
        candidates_modified.merge(
            std::move(candidates_by_block_number_.extract(cit).mapped()));
      }
      return candidates_modified;
    }

    void insert(primitives::BlockNumber block_number,
                CandidateHash candidate_hash) {
      ++candidates_.emplace(candidate_hash, 0).first;
      candidates_by_block_number_[block_number].emplace(candidate_hash);
    }
  };

  class Inclusions
      : private std::map<CandidateHash,
                         std::map<primitives::BlockNumber,
                                  std::vector<primitives::BlockHash>>> {
   public:
    // Add parent block to the vector which has CandidateHash as an outer key
    // and BlockNumber as an inner key
    void insert(CandidateHash candidate_hash,
                primitives::BlockNumber block_number,
                primitives::BlockHash block_hash) {
      auto &blocks_for_candidate = operator[](candidate_hash);
      auto &hashes_for_level = blocks_for_candidate[block_number];
      hashes_for_level.emplace_back(block_hash);
    }

    void remove_up_to_height(
        primitives::BlockNumber height,
        std::unordered_set<CandidateHash> candidates_modified) {
      for (auto &candidate : candidates_modified) {
        if (auto it = find(candidate); it != end()) {
          // Returns everything after the given key, including the key. This
          // works because the blocks are sorted in ascending order.
          auto &blocks_including = it->second;
          blocks_including.erase(blocks_including.begin(),
                                 blocks_including.upper_bound(height));
          if (blocks_including.empty()) {
            erase(it);
          }
        }
      }
    }

    std::vector<primitives::BlockInfo> get(
        const CandidateHash &candidate) const {
      std::vector<primitives::BlockInfo> inclusions_as_vec;
      if (auto it = find(candidate); it != end()) {
        auto &blocks_including = it->second;
        for (auto &[number, hashes] : blocks_including) {
          for (auto &hash : hashes) {
            inclusions_as_vec.emplace_back(number, hash);
          }
        }
      }
      return inclusions_as_vec;
    }
  };

  class ChainScraperImpl final : public ChainScraper {
   public:
    /// Limits the number of ancestors received for a single request.
    static constexpr uint32_t kAncestryChunkSize = 10;
    /// Limits the overall number of ancestors walked through for a given head.
    ///
    /// As long as we have `MAX_FINALITY_LAG` this makes sense as a value.
    static constexpr uint32_t kAncestrySizeLimit = 500;

    ChainScraperImpl(std::shared_ptr<runtime::ParachainHost> parachain_api,
                     std::shared_ptr<blockchain::BlockTree> block_tree);

    /// Check whether we have seen a candidate included on any chain.
    bool is_candidate_included(const CandidateHash &candidate_hash) override;
    /// Check whether the candidate is backed
    bool is_candidate_backed(const CandidateHash &candidate_hash) override;

    std::vector<primitives::BlockInfo> get_blocks_including_candidate(
        const CandidateHash &candidate_hash) override;

    outcome::result<ScrapedUpdates> process_active_leaves_update(
        const ActiveLeavesUpdate &update) override;

   private:
    /// Returns ancestors of `head` in the descending order, stopping
    /// either at the block present in cache or at the last finalized block.
    ///
    /// Both `head` and the latest finalized block are **not** included in the
    /// result.
    outcome::result<std::vector<primitives::BlockHash>>
    get_unfinalized_block_ancestors(primitives::BlockHash head,
                                    primitives::BlockNumber head_number);

    /// Process candidate events of a block.
    ///
    /// Keep track of all included and backed candidates.
    ///
    /// Returns freshly included candidate receipts
    outcome::result<std::vector<CandidateReceipt>> process_candidate_events(
        primitives::BlockNumber block_number, primitives::BlockHash block_hash);

    std::shared_ptr<runtime::ParachainHost> parachain_api_;
    std::shared_ptr<blockchain::BlockTree> block_tree_;

    /// All candidates we have seen included, which not yet have been finalized.
    ScrapedCandidates included_candidates_;

    /// All candidates we have seen backed
    ScrapedCandidates backed_candidates_;

    /// Latest relay blocks observed by the provider.
    ///
    /// We assume that ancestors of cached blocks are already processed, i.e. we
    /// have saved corresponding included candidates.
    std::unordered_map<primitives::BlockHash,
                       std::chrono::steady_clock::time_point>
        last_observed_blocks_;

    /// Maps included candidate hashes to one or more relay block heights and
    /// hashes. These correspond to all the relay blocks which marked a
    /// candidate as included, and are needed to apply reversions in case a
    /// dispute is concluded against the candidate.
    Inclusions inclusions_;
  };

}  // namespace kagome::dispute

#endif  // KAGOME_DISPUTE_CHAINSCRAPPERIMPL
