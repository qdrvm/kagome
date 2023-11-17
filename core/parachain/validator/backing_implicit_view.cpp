/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "parachain/validator/backing_implicit_view.hpp"

#include <gsl/span>

#include "parachain/types.hpp"

namespace kagome::parachain {
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

gsl::span<const Hash>
ImplicitView::knownAllowedRelayParentsUnder(
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

outcome::result<std::vector<ParachainId>>
ImplicitView::activate_leaf(const Hash &leaf_hash) {
  /// TODO(iceseer): do
  ///

  return std::vector<ParachainId>{};
}

}
