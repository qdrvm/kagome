/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "parachain/validator/backing_implicit_view.hpp"
#include "parachain/validator/prospective_parachains/prospective_parachains.hpp"

#include <span>

#include "parachain/types.hpp"
#include "primitives/math.hpp"

#define COMPONENT BackingImplicitView
#define COMPONENT_NAME STRINGIFY(COMPONENT)

OUTCOME_CPP_DEFINE_CATEGORY(kagome::parachain, ImplicitView::Error, e) {
  using E = decltype(e);
  switch (e) {
    case E::ALREADY_KNOWN:
      return COMPONENT_NAME ": Already known leaf";
    case E::NOT_INITIALIZED_WITH_PROSPECTIVE_PARACHAINS:
      return COMPONENT_NAME ": Not initialized with prospective parachains";
  }
  return COMPONENT_NAME ": unknown error";
}

namespace kagome::parachain {

  ImplicitView::ImplicitView(
      std::weak_ptr<ProspectiveParachains> prospective_parachains,
      std::shared_ptr<runtime::ParachainHost> parachain_host_,
      std::optional<ParachainId> collating_for_)
      : prospective_parachains_{std::move(prospective_parachains)},
        parachain_host(std::move(parachain_host_)),
        collating_for{std::move(collating_for_)} {
    BOOST_ASSERT(prospective_parachains_);
    BOOST_ASSERT(parachain_host);
  }

  std::span<const Hash>
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
        return std::span{allowed_relay_parents_contiguous}.first(slice_len);
      }
    }
    return {};
  }

  void ImplicitView::activate_leaf_from_prospective_parachains(
      fragment::BlockInfoProspectiveParachains leaf,
      const std::vector<fragment::BlockInfoProspectiveParachains> &ancestors) {
    if (leaves.contains(leaf.hash)) {
      return;
    }

    const auto retain_minimum =
        std::min(ancestors.empty() ? 0 : ancestors.back().number,
                 math::sat_sub_unsigned(leaf.number, MINIMUM_RETAIN_LENGTH));

    leaves.insert(leaf.hash,
                  ActiveLeafPruningInfo{
                      .retain_minimum = retain_minimum,
                  });
    AllowedRelayParents allowed_relay_parents{
        .minimum_relay_parents = {},
        .allowed_relay_parents_contiguous = {},
    };
    allowed_relay_parents.allowed_relay_parents_contiguous.reserve(
        ancestors.size());

    for (const auto &ancestor : ancestors) {
      block_info_storage.insert_or_assign(
          ancestor.hash,
          BlockInfo{
              .block_number = ancestor.number,
              .maybe_allowed_relay_parents = {},
              .parent_hash = ancestor.parent_hash,
          });
      allowed_relay_parents.allowed_relay_parents_contiguous.emplace_back(
          ancestor.hash);
    }

    block_info_storage.insert_or_assign(
        leaf.hash,
        BlockInfo{
            .block_number = leaf.number,
            .maybe_allowed_relay_parents = allowed_relay_parents,
            .parent_hash = leaf.parent_hash,
        });
  }

  std::span<const Hash> ImplicitView::knownAllowedRelayParentsUnder(
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

  std::vector<Hash> ImplicitView::deactivate_leaf(const Hash &leaf_hash) {
    std::vector<Hash> removed;
    if (leaves.erase(leaf_hash) == 0ull) {
      return removed;
    }

    std::optional<uint32_t> minimum;
    for (const auto &[_, l] : leaves) {
      minimum =
          minimum ? std::min(*minimum, l.retain_minimum) : l.retain_minimum;
    }

    for (auto it = block_info_storage.begin();
         it != block_info_storage.end();) {
      const auto &[hash, i] = *it;
      const bool keep = minimum && i.block_number >= *minimum;
      if (keep) {
        ++it;
      } else {
        removed.emplace_back(hash);
        it = block_info_storage.erase(it);
      }
    }
    return removed;
  }

  outcome::result<void> ImplicitView::activate_leaf(const Hash &leaf_hash) {
    if (leaves.contains(leaf_hash)) {
      return Error::ALREADY_KNOWN;
    }

    OUTCOME_TRY(fetched, fetch_fresh_leaf_and_insert_ancestry(leaf_hash));
    const uint32_t retain_minimum = std::min(
        fetched.minimum_ancestor_number,
        math::sat_sub_unsigned(fetched.leaf_number, MINIMUM_RETAIN_LENGTH));

    leaves.insert_or_assign(
        leaf_hash, ActiveLeafPruningInfo{.retain_minimum = retain_minimum});
    return outcome::success();
  }

  outcome::result<std::optional<BlockNumber>>
  fetch_min_relay_parents_for_collator(const Hash &leaf_hash,
                                       BlockNumber leaf_number) {
    size_t allowed_ancestry_len;
    if (auto mode =
            prospective_parachains_->prospectiveParachainsMode(leaf_hash)) {
      allowed_ancestry_len = mode->allowed_ancestry_len;
    } else {
      return std::nullopt;
    }

    BlockNumber min = leaf_number;
    OUTCOME_TRY(required_session,
                parachain_host->session_index_for_child(leaf_hash));
    OUTCOME_TRY(hashes,
                block_tree_->getDescendingChainToBlock(
                    leaf_hash, allowed_ancestry_len + 1));

    for (size_t i = 1; i < hashes.size(); ++i) {
      const auto &hash = hashes[i];
      OUTCOME_TRY(session, parachain_host->session_index_for_child(hash));

      if (session == required_session) {
        min = math::sat_sub_unsigned(min, 1);
      } else {
        break;
      }
    }

    return min;
  }

  outcome::result<ImplicitView::FetchSummary>
  ImplicitView::fetch_fresh_leaf_and_insert_ancestry(const Hash &leaf_hash) {
    std::shared_ptr<blockchain::BlockTree> block_tree =
        prospective_parachains_->getBlockTree();

    OUTCOME_TRY(leaf_header, block_tree->getBlockHeader(leaf_hash));

    std::vector<std::pair<ParachainId, BlockNumber>> min_relay_parents;
    if (collating_for) {
      OUTCOME_TRY(
          mrp,
          fetch_min_relay_parents_for_collator(leaf_hash, leaf_header.number));
      if (mrp) {
        min_relay_parents.emplace_back(*collating_for, *mrp);
      }
    } else {
      min_relay_parents =
          prospective_parachains_->answerMinimumRelayParentsRequest(leaf_hash);
    }

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
