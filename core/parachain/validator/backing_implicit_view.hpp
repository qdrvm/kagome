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
          const BlockNumber &base_number) const {
        if (!para_id) {
          return {allowed_relay_parents_contiguous};
        }

        if (auto it = minimum_relay_parents.find(*para_id);
            it != minimum_relay_parents.end()) {
          const BlockNumber &para_min = it->second;
          if (base_number >= para_min) {
            const auto diff = base_number - para_min;
            const size_t slice_len = std::min(
                size_t(diff + 1), allowed_relay_parents_contiguous.size());
            if (slice_len > 0ull) {
              return gsl::span<const Hash>(
                  &allowed_relay_parents_contiguous[0ull], slice_len);
            }
          }
        }
        return {};
      }
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
        const std::optional<ParachainId> &para_id) const {
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
  };

}  // namespace kagome::parachain

#endif  // KAGOME_PARACHAIN_BACKING_IMPLICIT_VIEW_HPP
