/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "parachain/validator/backing_implicit_view.hpp"

#include <gsl/span>

#include "parachain/types.hpp"
#include "primitives/math.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::parachain, ImplicitView::Error, e) {
  using E = decltype(e);
  switch (e) {
    case E::ALREADY_KNOWN:
      return "Already known leaf";
  }
  return fmt::format("ImplicitView failed. (error={})", e);
}

namespace kagome::parachain {

  ImplicitView::ImplicitView(
      std::shared_ptr<ProspectiveParachains> prospective_parachains)
      : prospective_parachains_{std::move(prospective_parachains)} {
    BOOST_ASSERT(prospective_parachains_);
  }

  gsl::span<const Hash>
  ImplicitView::AllowedRelayParents::allowedRelayParentsFor(
      const std::optional<ParachainId> &para_id,
      const BlockNumber &base_number) const {
    if (!para_id) {
      return {allowed_relay_parents_contiguous};
    }

    if (auto it = minimum_relay_parents.find(*para_id);
        it != minimum_relay_parents.end()) {
      const BlockNumber &para_min = it->second;
      if (base_number >= para_min) {
        const auto diff = base_number - para_min;
        const size_t slice_len =
            std::min(size_t(diff + 1), allowed_relay_parents_contiguous.size());
        if (slice_len > 0ull) {
          return gsl::span<const Hash>(&allowed_relay_parents_contiguous[0ull],
                                       slice_len);
        }
      }
    }
    return {};
  }

  gsl::span<const Hash> ImplicitView::knownAllowedRelayParentsUnder(
      const Hash &block_hash, const std::optional<ParachainId> &para_id) const {
    if (auto it = block_info_storage.find(block_hash);
        it != block_info_storage.end()) {
      const BlockInfo &block_info = it->second;
      if (block_info.maybe_allowed_relay_parents) {
        return block_info.maybe_allowed_relay_parents->allowedRelayParentsFor(
            para_id, block_info.block_number);
      }
    }
    return {};
  }

  outcome::result<std::vector<ParachainId>> ImplicitView::activate_leaf(
      const Hash &leaf_hash) {
    if (leaves.find(leaf_hash) != leaves.end()) {
      return Error::ALREADY_KNOWN;
    }

    OUTCOME_TRY(fetched, fetch_fresh_leaf_and_insert_ancestry(leaf_hash));
    const uint32_t retain_minimum = std::min(
        fetched.minimum_ancestor_number,
        math::sat_sub_unsigned(fetched.leaf_number, MINIMUM_RETAIN_LENGTH));

    leaves.emplace(leaf_hash,
                   ActiveLeafPruningInfo{.retain_minimum = retain_minimum});
    return fetched.relevant_paras;
  }

  outcome::result<FetchSummary>
  ImplicitView::fetch_fresh_leaf_and_insert_ancestry(const Hash &leaf_hash) {
    std::vector<std::pair<ParachainId, BlockNumber>> min_relay_parents_raw =
        prospective_parachains_->answerMinimumRelayParentsRequest(leaf_hash);
    std::shared_ptr<blockchain::BlockTree> block_tree =
        prospective_parachains_->getBlockTree();

    OUTCOME_TRY(leaf_header, block_tree->getBlockHeader(leaf_hash));
    BlockNumber min_min = min_relay_parents_raw.empty()
                            ? leaf_header.number
                            : min_relay_parents_raw[0].second;
    std::vector<ParachainId> relevant_paras;
    relevant_paras.reserve(min_relay_parents_raw.size());

    for (size_t i = 0; i < min_relay_parents_raw.size(); ++i) {
      min_min = std::min(min_relay_parents_raw[i].second, min_min);
      relevant_paras.emplace_back(min_relay_parents_raw[i].first);
    }

    const size_t expected_ancestry_len =
        math::sat_sub_unsigned(leaf_header.number, min_min) + 1ull;
    std::vector<Hash> ancestry;
    if (leaf_header.number > 0) {
      BlockNumber next_ancestor_number = leaf_header.number - 1;
      Hash next_ancestor_hash = leaf_header.parent_hash;

      ancestry.reserve(expected_ancestry_len);
      ancestry.emplace_back(leaf_hash);

      while (next_ancestor_number >= min_min) {
        auto it = block_info_storage.find(next_ancestor_hash);
        std::optional<Hash> parent_hash;
        if (it != block_info_storage.end()) {
          parent_hash = it->second.parent_hash;
        } else {
          OUTCOME_TRY(header, block_tree->getBlockHeader(next_ancestor_hash));
          block_info_storage.emplace(
              next_ancestor_hash,
              BlockInfo{
                  .block_number = next_ancestor_number,
                  .maybe_allowed_relay_parents = std::nullopt,
                  .parent_hash = header.parent_hash,
              });
          parent_hash = header.parent_hash;
        }

        ancestry.emplace_back(next_ancestor_hash);
        if (next_ancestor_number == 0) {
          break;
        }

        next_ancestor_number -= 1;
        next_ancestor_hash = *parent_hash;
      }
    } else {
      ancestry.emplace_back(leaf_hash);
    }

    block_info_storage.emplace(
        leaf_hash,
        BlockInfo{
            .block_number = leaf_header.number,
            .maybe_allowed_relay_parents =
                AllowedRelayParents{
                    .minimum_relay_parents = {min_relay_parents_raw.begin(),
                                              min_relay_parents_raw.end()},
                    .allowed_relay_parents_contiguous = std::move(ancestry),
                },
            .parent_hash = leaf_header.parent_hash,
        });
    return FetchSummary{
        .minimum_ancestor_number = min_min,
        .leaf_number = leaf_header.number,
        .relevant_paras = relevant_paras,
    };
  }

}  // namespace kagome::parachain
