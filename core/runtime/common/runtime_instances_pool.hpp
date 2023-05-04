/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_INSTANCES_POOL_HPP
#define KAGOME_CORE_RUNTIME_INSTANCES_POOL_HPP

#include "runtime/module_repository.hpp"

#include <stack>
#include <mutex>

namespace kagome::runtime {
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

    std::optional<std::reference_wrapper<const Value>> get(const Key &key) {
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
      return std::nullopt;
    }

    template <typename ValueArg>
    void put(const Key &key, ValueArg &&value) {
      static_assert(std::is_convertible_v<
                        ValueArg,
                        Value> || std::is_constructible_v<ValueArg, Value>);
      ticks_++;
      if (cache_.size() >= kMaxSize) {
        auto min = std::min_element(cache_.begin(), cache_.end());
        cache_.erase(min);
      }
      cache_.push_back(CacheEntry{key, std::forward<ValueArg>(value), ticks_});
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

  /**
   * @brief Pool of runtime instances - per state. Incapsulates modules cache.
   *
   */
  class RuntimeInstancesPool final
      : public std::enable_shared_from_this<RuntimeInstancesPool> {
    using ModuleInstancePool = std::stack<std::shared_ptr<ModuleInstance>>;

   public:
    using RootHash = storage::trie::RootHash;
    using ModuleCache =
        SmallLruCache<storage::trie::RootHash, std::shared_ptr<Module>>;

    /**
     * @brief Instantiate new or reuse existing ModuleInstance for the provided
     * state.
     *
     * @param state - the merkle trie root of the state containing the code of
     * the runtime module we are acquiring an instance of.
     * @return pointer to the acquired ModuleInstance if success. Error
     * otherwise.
     */
    outcome::result<std::shared_ptr<ModuleInstance>> tryAcquire(
        const RootHash &state);
    /**
     * @brief Releases the module instance (returns it to the pool)
     *
     * @param state - the merkle trie root of the state containing the runtime
     * module code we are releasing an instance of.
     * @param instance - instance to be released.
     */
    void release(const RootHash &state,
                 std::shared_ptr<ModuleInstance> &&instance);

    /**
     * @brief Get the module for state from internal cache
     *
     * @param state - the state containing the module's code.
     * @return Module if any, nullopt otherwise
     */
    std::optional<std::shared_ptr<Module>> getModule(const RootHash &state);

    /**
     * @brief Puts new module into internal cache
     *
     * @param state - runtime block, by its root hash
     * @param module - new module pointer
     */
    void putModule(const RootHash &state, std::shared_ptr<Module> module);

   private:
    std::mutex mt_;
    static constexpr size_t MODULES_CACHE_SIZE = 2;
    ModuleCache modules_{MODULES_CACHE_SIZE};
    std::map<RootHash, ModuleInstancePool> pools_;
  };

}  // namespace kagome::runtime

#endif  // KAGOME_CORE_RUNTIME_INSTANCES_POOL_HPP
