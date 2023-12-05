/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/lru_cache.hpp"
#include "network/types/grandpa_message.hpp"

#include <unordered_set>

namespace kagome::consensus::grandpa {
  struct VotesCacheItem {
    size_t type;
    Id id;
    primitives::BlockHash block_hash;

    bool operator==(const VotesCacheItem &rhs) const = default;
  };
}  // namespace kagome::consensus::grandpa

template <>
struct std::hash<kagome::consensus::grandpa::VotesCacheItem> {
  std::size_t operator()(
      const kagome::consensus::grandpa::VotesCacheItem &item) const {
    size_t result = std::hash<size_t>()(item.type);
    boost::hash_combine(result,
                        std::hash<kagome::consensus::grandpa::Id>()(item.id));
    boost::hash_combine(
        result, std::hash<kagome::primitives::BlockHash>()(item.block_hash));
    return result;
  }
};

namespace kagome::consensus::grandpa {

  /**
   * Cache for vote messages. Internally uses LRU cache where round number
   * is key and set of vote messages during this round is value
   */
  class VotesCache {
   public:
    /**
     * @param size – The number of most recent rounds for which vote records are
     * retained.
     */
    explicit VotesCache(size_t size);

    void put(const VoteMessage &msg);

    bool contains(const VoteMessage &msg);

   private:
    LruCache<RoundNumber, std::unordered_set<VotesCacheItem>> lru_cache_;
  };
}  // namespace kagome::consensus::grandpa
