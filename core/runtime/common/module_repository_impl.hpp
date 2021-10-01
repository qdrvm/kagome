/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_MODULE_REPOSITORY_IMPL_HPP
#define KAGOME_CORE_RUNTIME_MODULE_REPOSITORY_IMPL_HPP

#include "runtime/module_repository.hpp"

#include <unordered_map>

#include "log/logger.hpp"
#include "runtime/instance_environment.hpp"

namespace kagome::runtime {

  class RuntimeUpgradeTracker;
  class ModuleFactory;

  /**
   * LRU cache designed for small amounts of data (as its get() is O(N))
   */
  template <typename Key, typename Value, typename PriorityType = uint64_t>
  struct SmallLruCache final {
   public:
    static_assert(std::is_unsigned_v<PriorityType>);

    struct CacheEntry {
      Key key;
      Value value;
      PriorityType latest_use_tick_;

      bool operator<(const CacheEntry &rhs) const {
        return latest_use_tick_ < rhs.latest_use_tick_;
      }
    };

    SmallLruCache(size_t max_size) : kMaxSize{max_size} {
      BOOST_ASSERT(kMaxSize > 0);
      cache_.reserve(kMaxSize);
    }

    boost::optional<const Value &> get(const Key &key) {
      ticks_++;
      if (ticks_ == 0) {
        handleTicksOverflow();
      }
      for (auto &entry : cache_) {
        if (entry.key == key) {
          entry.latest_use_tick_ = ticks_;
          return entry.value;
        }
      }
      return boost::none;
    }

    template <typename ValueArg>
    [[nodiscard]] bool put(const Key &key, ValueArg &&value) {
      static_assert(std::is_convertible_v<
                        ValueArg,
                        Value> || std::is_constructible_v<ValueArg, Value>);
      ticks_++;
      if (cache_.size() >= kMaxSize) {
        auto min = std::min_element(cache_.begin(), cache_.end());
        cache_.erase(min);
      }
      cache_.push_back(CacheEntry{key, std::forward<ValueArg>(value), ticks_});
      return true;
    }

   private:
    void handleTicksOverflow() {
      // 'compress' timestamps of entries in the cache (works because we care
      // only about their order, not actual timestamps)
      std::sort(cache_.begin(), cache_.end());
      for (auto &entry : cache_) {
        entry.latest_use_tick_ = ticks_;
        ticks_++;
      }
    }

    const size_t kMaxSize;
    // an abstract representation of time to implement 'recency' without
    // depending on real time. Incremented on each cache access
    PriorityType ticks_{};
    std::vector<CacheEntry> cache_;
  };

  class ModuleRepositoryImpl final : public ModuleRepository {
   public:
    ModuleRepositoryImpl(
        std::shared_ptr<RuntimeUpgradeTracker> runtime_upgrade_tracker,
        std::shared_ptr<const ModuleFactory> module_factory);

    outcome::result<std::shared_ptr<ModuleInstance>> getInstanceAt(
        std::shared_ptr<const RuntimeCodeProvider> code_provider,
        const primitives::BlockInfo &block) override;

   private:
    static constexpr size_t MODULES_CACHE_SIZE = 2;
    static constexpr size_t INSTANCES_CACHE_SIZE = 2;

    SmallLruCache<storage::trie::RootHash, std::shared_ptr<Module>> modules_;
    std::mutex modules_mutex_;

    static thread_local SmallLruCache<storage::trie::RootHash,
                                      std::shared_ptr<ModuleInstance>>
        instances_cache_;

    std::mutex instances_mutex_;
    std::shared_ptr<RuntimeUpgradeTracker> runtime_upgrade_tracker_;
    std::shared_ptr<const ModuleFactory> module_factory_;
    log::Logger logger_;
  };

}  // namespace kagome::runtime

#endif  // KAGOME_CORE_RUNTIME_MODULE_REPOSITORY_IMPL_HPP
