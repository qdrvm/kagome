/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <optional>
#include <outcome/outcome.hpp>
#include <span>
#include <unordered_map>
#include <vector>

#include "blockchain/block_tree.hpp"
#include "blockchain/block_tree_error.hpp"
#include "parachain/types.hpp"
#include "parachain/validator/prospective_parachains/common.hpp"
#include "primitives/common.hpp"
#include "runtime/runtime_api/parachain_host.hpp"
#include "runtime/runtime_api/parachain_host_types.hpp"

namespace kagome::parachain {

  class ProspectiveParachains;

  // Always aim to retain 1 block before the active leaves.
  constexpr BlockNumber MINIMUM_RETAIN_LENGTH = 2ull;

  struct ImplicitView {
    enum Error : uint8_t {
      ALREADY_KNOWN,
      NOT_INITIALIZED_WITH_PROSPECTIVE_PARACHAINS
    };

    struct FetchSummary {
      BlockNumber minimum_ancestor_number;
      BlockNumber leaf_number;
    };

    /// Get the known, allowed relay-parents that are valid for parachain
    /// candidates which could be backed in a child of a given block for a given
    /// para ID.
    ///
    /// This is expressed as a contiguous slice of relay-chain block hashes
    /// which may include the provided block hash itself.
    ///
    /// If `para_id` is `None`, this returns all valid relay-parents across all
    /// paras for the leaf.
    ///
    /// `None` indicates that the block hash isn't part of the implicit view or
    /// that there are no known allowed relay parents.
    ///
    /// This always returns `Some` for active leaves or for blocks that
    /// previously were active leaves.
    ///
    /// This can return the empty slice, which indicates that no relay-parents
    /// are allowed for the para, e.g. if the para is not scheduled at the given
    /// block hash.
    std::optional<std::span<const Hash>> known_allowed_relay_parents_under(
        const Hash &block_hash,
        const std::optional<ParachainId> &para_id) const;

    /// Activate a leaf in the view. To be used by the prospective parachains
    /// subsystem.
    ///
    /// This will not request any additional data, as prospective parachains
    /// already provides all the required info. NOTE: using `activate_leaf`
    /// instead of this function will result in a deadlock, as it calls
    /// prospective-parachains under the hood.
    ///
    /// No-op for known leaves.
    void activate_leaf_from_prospective_parachains(
        fragment::BlockInfoProspectiveParachains leaf,
        const std::vector<fragment::BlockInfoProspectiveParachains> &ancestors);

    /// Activate a leaf in the view.
    /// This will request the minimum relay parents the leaf and will load
    /// headers in the ancestry of the leaf as needed. These are the 'implicit
    /// ancestors' of the leaf.
    ///
    /// To maximize reuse of outdated leaves, it's best to activate new leaves
    /// before deactivating old ones.
    ///
    /// The allowed relay parents for the relevant paras under this leaf can be
    /// queried with [`View::known_allowed_relay_parents_under`].
    ///
    /// No-op for known leaves.
    outcome::result<void> activate_leaf(const Hash &leaf_hash);

    /// Deactivate a leaf in the view. This prunes any outdated implicit
    /// ancestors as well.
    ///
    /// Returns hashes of blocks pruned from storage.
    std::vector<Hash> deactivate_leaf(const Hash &leaf_hash);

    /// Get an iterator over all allowed relay-parents in the view with no
    /// particular order.
    ///
    /// **Important**: not all blocks are guaranteed to be allowed for some
    /// leaves, it may happen that a block info is only kept in the view storage
    /// because of a retaining rule.
    ///
    /// For getting relay-parents that are valid for parachain candidates use
    /// [`View::known_allowed_relay_parents_under`].
    std::vector<Hash> all_allowed_relay_parents() const {
      std::vector<Hash> r;
      r.reserve(block_info_storage.size());
      for (const auto &[h, _] : block_info_storage) {
        r.emplace_back(h);
      }
      return r;
    }

    /// Trace print of all internal buffers.
    ///
    /// Usable for tracing memory consumption.
    void printStoragesLoad() {
      SL_TRACE(logger,
               "[Backing implicit view statistics]:"
               "\n\t-> leaves={}"
               "\n\t-> block_info_storage={}",
               leaves.size(),
               block_info_storage.size());
    }

    ImplicitView(std::weak_ptr<ProspectiveParachains> prospective_parachains,
                 std::shared_ptr<runtime::ParachainHost> parachain_host_,
                 std::shared_ptr<blockchain::BlockTree> block_tree,
                 std::optional<ParachainId> collating_for_);

   private:
    struct ActiveLeafPruningInfo {
      BlockNumber retain_minimum;
    };

    struct AllowedRelayParents {
      std::unordered_map<ParachainId, BlockNumber> minimum_relay_parents;
      std::vector<Hash> allowed_relay_parents_contiguous;

      std::span<const Hash> allowedRelayParentsFor(
          const std::optional<ParachainId> &para_id,
          const BlockNumber &base_number) const;
    };

    struct BlockInfo {
      BlockNumber block_number;
      std::optional<AllowedRelayParents> maybe_allowed_relay_parents;
      Hash parent_hash;
    };

    outcome::result<FetchSummary> fetch_fresh_leaf_and_insert_ancestry(
        const Hash &leaf_hash);

    outcome::result<std::optional<BlockNumber>>
    fetch_min_relay_parents_for_collator(const Hash &leaf_hash,
                                         BlockNumber leaf_number);

    std::unordered_map<Hash, ActiveLeafPruningInfo> leaves;
    std::unordered_map<Hash, BlockInfo> block_info_storage;
    std::shared_ptr<runtime::ParachainHost> parachain_host;
    std::optional<ParachainId> collating_for;

    std::weak_ptr<ProspectiveParachains> prospective_parachains_;
    std::shared_ptr<blockchain::BlockTree> block_tree_;
    log::Logger logger = log::createLogger("BackingImplicitView", "parachain");
  };

}  // namespace kagome::parachain

OUTCOME_HPP_DECLARE_ERROR(kagome::parachain, ImplicitView::Error)
