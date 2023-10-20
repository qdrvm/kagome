/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/grandpa/impl/votes_cache.hpp"

namespace kagome::consensus::grandpa {
  VotesCache::VotesCache(size_t size) : lru_cache_(size) {}

  void VotesCache::put(const VoteMessage &msg) {
    auto round = msg.round_number;
    auto type = msg.vote.message.type().hash_code();
    if (auto opt_set = lru_cache_.get(round); opt_set) {
      std::shared_ptr<std::unordered_set<VotesCacheItem>> non_const =
          std::const_pointer_cast<std::unordered_set<VotesCacheItem>>(
              opt_set.value());
      non_const->insert({type, msg.id(), msg.vote.getBlockHash()});
    } else {
      std::unordered_set<VotesCacheItem> votes;
      votes.insert({type, msg.id(), msg.vote.getBlockHash()});
      lru_cache_.put(round, std::move(votes));
    }
  }

  bool VotesCache::contains(VoteMessage msg) {
    if (auto opt_set = lru_cache_.get(msg.round_number); opt_set) {
      auto type = msg.vote.message.type().hash_code();
      return opt_set.value()->contains(
          {type, msg.id(), msg.vote.getBlockHash()});
    }
    return false;
  }
}  // namespace kagome::consensus::grandpa
