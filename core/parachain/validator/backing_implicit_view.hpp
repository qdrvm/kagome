/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_PARACHAIN_BACKING_IMPLICIT_VIEW_HPP
#define KAGOME_PARACHAIN_BACKING_IMPLICIT_VIEW_HPP

#include <gsl/span>
#include <optional>
#include <unordered_map>
#include <vector>
#include "parachain/types.hpp"
#include "primitives/common.hpp"

namespace kagome::parachain {

  // Always aim to retain 1 block before the active leaves.
  constexpr BlockNumber MINIMUM_RETAIN_LENGTH = 2ull;

  struct ImplicitView {
    struct ActiveLeafPruningInfo {
      BlockNumber retain_minimum;
    };

    struct AllowedRelayParents {
      std::unordered_map<ParachainId, BlockNumber> minimum_relay_parents;
      std::vector<Hash> allowed_relay_parents_contiguous;

      gsl::span<const Hash> allowedRelayParentsFor(
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

    gsl::span<const Hash> knownAllowedRelayParentsUnder(
        const Hash &block_hash,
        const std::optional<ParachainId> &para_id) const;
    outcome::result<std::vector<ParachainId>> activate_leaf(
        const Hash &leaf_hash);
  };

}  // namespace kagome::parachain

#endif  // KAGOME_PARACHAIN_BACKING_IMPLICIT_VIEW_HPP
