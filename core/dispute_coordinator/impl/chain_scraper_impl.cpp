/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "dispute_coordinator/impl/chain_scraper_impl.hpp"

#include "runtime/runtime_api/parachain_host.hpp"

namespace kagome::dispute {

  ChainScraperImpl::ChainScraperImpl(
      std::shared_ptr<runtime::ParachainHost> parachain_api,
      std::shared_ptr<blockchain::BlockTree> block_tree,
      std::shared_ptr<crypto::Hasher> hasher)
      : parachain_api_(std::move(parachain_api)),
        block_tree_(std::move(block_tree)),
        hasher_(std::move(hasher)) {
    BOOST_ASSERT(parachain_api_);
    BOOST_ASSERT(block_tree_);
    BOOST_ASSERT(hasher_);
  }

  bool ChainScraperImpl::is_candidate_included(
      const CandidateHash &candidate_hash) {
    return included_candidates_.contains(candidate_hash);
  }

  bool ChainScraperImpl::is_candidate_backed(
      const CandidateHash &candidate_hash) {
    return backed_candidates_.contains(candidate_hash);
  }

  std::vector<primitives::BlockInfo>
  ChainScraperImpl::get_blocks_including_candidate(
      const CandidateHash &candidate_hash) {
    return inclusions_.get(candidate_hash);
  }

  // https://github.com/paritytech/polkadot/blob/40974fb99c86f5c341105b7db53c7aa0df707d66/node/core/dispute-coordinator/src/scraping/mod.rs#L288
  outcome::result<ScrapedUpdates>
  ChainScraperImpl::process_active_leaves_update(
      const ActiveLeavesUpdate &update) {
    if (not update.activated.has_value()) {
      return ScrapedUpdates{};
    }
    const auto &activated = update.activated.value();

    // Fetch ancestry up to last finalized block.
    OUTCOME_TRY(ancestors,
                get_unfinalized_block_ancestors(
                    /*sender,*/ activated.hash, activated.number));

    ScrapedUpdates scraped_updates;

    // Ancestors block numbers are consecutive in the descending order.
    for (size_t i = 0; i < ancestors.size(); ++i) {
      const auto block_number = activated.number - i;
      const auto &block_hash = ancestors[i];

      // LOG-TRACE: "In ancestor processing."

      OUTCOME_TRY(receipts_for_block,
                  process_candidate_events(block_number, block_hash));

      scraped_updates.included_receipts.reserve(
          scraped_updates.included_receipts.size() + receipts_for_block.size());
      scraped_updates.included_receipts.insert(
          scraped_updates.included_receipts.end(),
          std::move_iterator(receipts_for_block.begin()),
          std::move_iterator(receipts_for_block.end()));

      OUTCOME_TRY(votes_opt, parachain_api_->on_chain_votes(block_hash));

      if (votes_opt.has_value()) {
        auto &votes = votes_opt.value();
        scraped_updates.on_chain_votes.emplace_back(std::move(votes));
      }
    }

    last_observed_blocks_.put(activated.hash, Empty{});

    return scraped_updates;
  }

  outcome::result<void> ChainScraperImpl::process_finalized_block(
      const primitives::BlockInfo &finalized) {
    // `kDisputeCandidateLifetimeAfterFinalization - 1` because
    // `finalized_block_number` counts to the candidate lifetime.

    if (finalized.number < kDisputeCandidateLifetimeAfterFinalization - 1) {
      // Nothing to prune. We are still in the beginning of the chain and there
      // are not enough finalized blocks yet.
      return outcome::success();
    }

    auto key_to_prune =
        finalized.number - (kDisputeCandidateLifetimeAfterFinalization - 1);

    backed_candidates_.remove_up_to_height(key_to_prune);
    auto candidates_modified =
        included_candidates_.remove_up_to_height(key_to_prune);
    inclusions_.remove_up_to_height(key_to_prune, candidates_modified);

    return outcome::success();
  }

  // https://github.com/paritytech/polkadot/blob/40974fb99c86f5c341105b7db53c7aa0df707d66/node/core/dispute-coordinator/src/scraping/mod.rs#L336
  outcome::result<std::vector<primitives::BlockHash>>
  ChainScraperImpl::get_unfinalized_block_ancestors(
      primitives::BlockHash head, primitives::BlockNumber head_number) {
    auto target_ancestor = block_tree_->getLastFinalized().number;

    std::vector<primitives::BlockHash> ancestors;

    // If head_number <= target_ancestor + 1 the ancestry will be empty.
    if (last_observed_blocks_.get(head).has_value()
        or head_number <= target_ancestor + 1) {
      return ancestors;
    }

    for (;;) {
      OUTCOME_TRY(
          hashes,
          block_tree_->getDescendingChainToBlock(head, kAncestryChunkSize));

      if (hashes.empty()) {
        break;
      }

      if (head_number < hashes.size()) {
        // It's assumed that it's impossible to retrieve
        // more than N ancestors for block number N.

        // LOG-ERR: "Received {} ancestors for block number {} from Chain API",
        //           hashes.len(),
        //           head_number

        return ancestors;
      }

      auto earliest_block_number = head_number - hashes.size();

      // The reversed order is parent, grandparent, etc. excluding the head.
      for (size_t i = 0; i < hashes.size(); ++i) {
        const auto block_number = head_number - i - 1;
        const auto &block_hash = hashes[i];
        if (block_hash == head) {
          continue;
        }

        // Return if we either met target/cached block or
        // hit the size limit for the returned ancestry of head.
        if (last_observed_blocks_.get(block_hash).has_value()
            or block_number <= target_ancestor
            or ancestors.size() >= kAncestrySizeLimit) {
          return ancestors;
        }
        ancestors.push_back(block_hash);
      }

      head = hashes.back();
      head_number = earliest_block_number;
    }

    return ancestors;
  }

  outcome::result<std::vector<CandidateReceipt>>
  ChainScraperImpl::process_candidate_events(
      primitives::BlockNumber block_number, primitives::BlockHash block_hash) {
    OUTCOME_TRY(events, parachain_api_->candidate_events(block_hash));

    std::vector<CandidateReceipt> included_receipts;

    // Get included and backed events:
    for (const auto &event : events) {
      visit_in_place(
          event,
          [&](const runtime::CandidateIncluded &ev) {
            auto &receipt = ev.candidate_receipt;
            auto &candidate_hash = receipt.hash(*hasher_);
            // LOG-TRACE: "Processing included event"
            included_candidates_.insert(block_number, candidate_hash);
            inclusions_.insert(candidate_hash, block_number, block_hash);
            included_receipts.push_back(receipt);
          },
          [&](const runtime::CandidateBacked &ev) {
            auto &receipt = ev.candidate_receipt;
            auto &candidate_hash = receipt.hash(*hasher_);
            // LOG-TRACE: "Processing backed event"
            backed_candidates_.insert(block_number, candidate_hash);
          },
          [](const auto &) {
            // skip the rest
          });
    }

    return included_receipts;
  }

}  // namespace kagome::dispute
