/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <optional>
#include <span>
#include <unordered_map>
#include <vector>

#include "parachain/types.hpp"
#include "parachain/validator/prospective_parachains.hpp"
#include "primitives/common.hpp"

namespace kagome::parachain {

  // Always aim to retain 1 block before the active leaves.
  constexpr BlockNumber MINIMUM_RETAIN_LENGTH = 2ull;

  struct ImplicitView {
    enum Error {
      ALREADY_KNOWN,
    };

    struct FetchSummary {
      BlockNumber minimum_ancestor_number;
      BlockNumber leaf_number;
      std::vector<ParachainId> relevant_paras;
    };

    std::span<const Hash> knownAllowedRelayParentsUnder(
        const Hash &block_hash,
        const std::optional<ParachainId> &para_id) const;
    outcome::result<std::vector<ParachainId>> activate_leaf(
        const Hash &leaf_hash);
    std::vector<Hash> deactivate_leaf(const Hash &leaf_hash);
    std::vector<Hash> all_allowed_relay_parents() const {
      std::vector<Hash> r;
      r.reserve(block_info_storage.size());
      for (const auto &[h, _] : block_info_storage) {
        r.emplace_back(h);
      }
      return r;
    }

    void printStoragesLoad() {
      SL_TRACE(logger,
               "[Backing implicit view statistics]:"
               "\n\t-> leaves={}"
               "\n\t-> block_info_storage={}",
               leaves.size(),
               block_info_storage.size());
    }

    ImplicitView(std::shared_ptr<ProspectiveParachains> prospective_parachains);

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

    std::unordered_map<Hash, ActiveLeafPruningInfo> leaves;
    std::unordered_map<Hash, BlockInfo> block_info_storage;
    std::shared_ptr<ProspectiveParachains> prospective_parachains_;
    log::Logger logger = log::createLogger("BackingImplicitView", "parachain");

    outcome::result<FetchSummary> fetch_fresh_leaf_and_insert_ancestry(
        const Hash &leaf_hash);
  };

}  // namespace kagome::parachain

OUTCOME_HPP_DECLARE_ERROR(kagome::parachain, ImplicitView::Error)
