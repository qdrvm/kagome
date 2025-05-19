#pragma once

#include <boost/functional/hash.hpp>
#include <compare>
#include "parachain/types.hpp"
#include "primitives/common.hpp"

namespace kagome::parachain {

  /**
   * @brief Identifies collations blocked from seconding because parent head
   * data is not available
   */
  struct BlockedCollationId {
    /// Para id.
    ParachainId para_id;
    /// Hash of the parent head data.
    Hash parent_head_data_hash;

    BlockedCollationId(ParachainId pid, const Hash &h)
        : para_id(pid), parent_head_data_hash(h) {}
    auto operator<=>(const BlockedCollationId &) const = default;
    bool operator==(const BlockedCollationId &) const = default;
  };

}  // namespace kagome::parachain

// Note: std::hash specialization is now in blocked_collation_id_hash.hpp
