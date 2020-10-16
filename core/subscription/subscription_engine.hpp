/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_SUBSCRIPTION_ENGINE_HPP
#define KAGOME_SUBSCRIPTION_ENGINE_HPP

#include <list>
#include <memory>
#include <shared_mutex>
#include <unordered_map>

namespace kagome::subscription {
  template <typename Key, typename Type, typename... Arguments>
  class Subscriber;

  template <typename Key, typename Type, typename... Arguments>
  class SubscriptionEngine final
      : public std::enable_shared_from_this<
            SubscriptionEngine<Key, Type, Arguments...>> {
   public:
    using KeyType = Key;
    using ValueType = Type;
    using SubscriberType = Subscriber<KeyType, ValueType, Arguments...>;
    using SubscriberPtr = std::shared_ptr<SubscriberType>;
    using SubscriberWPtr = std::weak_ptr<SubscriberType>;
    using SubscriptionSetId = uint32_t;

    /// List is preferable here because this container iterators remain
    /// alive after removal from the middle of the container
    /// TODO(iceseer): PRE-476 remove processor cache penalty, while iterating, using
    /// custom allocator
    using SubscribersContainer = std::list<std::pair<SubscriptionSetId,SubscriberWPtr>>;
    using IteratorType = typename SubscribersContainer::iterator;

   private:
    template <typename KeyType, typename ValueType, typename... Args>
    friend class Subscriber;
    using KeyValueContainer = std::unordered_map<KeyType, SubscribersContainer>;

    mutable std::shared_mutex subscribers_map_cs_;
    KeyValueContainer subscribers_map_;

   public:
    SubscriptionEngine() = default;
    ~SubscriptionEngine() = default;

    SubscriptionEngine(SubscriptionEngine &&) = default; // NOLINT
    SubscriptionEngine &operator=(SubscriptionEngine &&) = default; // NOLINT

    SubscriptionEngine(const SubscriptionEngine &) = delete;
    SubscriptionEngine &operator=(const SubscriptionEngine &) = delete;

   private:
    IteratorType subscribe(SubscriptionSetId set_id, const KeyType &key, SubscriberWPtr ptr) {
      std::unique_lock lock(subscribers_map_cs_);
      auto &subscribers_list = subscribers_map_[key];
      return subscribers_list.emplace(subscribers_list.end(), std::make_pair(set_id, std::move(ptr)));
    }

    void unsubscribe(const KeyType &key, const IteratorType &it_remove) {
      std::unique_lock lock(subscribers_map_cs_);
      auto it = subscribers_map_.find(key);
      if (subscribers_map_.end() != it) {
        it->second.erase(it_remove);
        if (it->second.empty()) subscribers_map_.erase(it);
      }
    }

   public:
    size_t size(const KeyType &key) const {
      std::shared_lock lock(subscribers_map_cs_);
      if (auto it = subscribers_map_.find(key); it != subscribers_map_.end())
        return it->second.size();

      return 0ull;
    }

    size_t size() const {
      std::shared_lock lock(subscribers_map_cs_);
      size_t count = 0ull;
      for (auto &it : subscribers_map_) count += it.second.size();
      return count;
    }

    void notify(const KeyType &key, const Arguments &... args) {
      std::shared_lock lock(subscribers_map_cs_);
      auto it = subscribers_map_.find(key);
      if (subscribers_map_.end() == it) return;

      auto &subscribers_container = it->second;
      for (auto it_sub = subscribers_container.begin();
           it_sub != subscribers_container.end();) {
        if (auto sub = it_sub->second.lock()) {
          sub->on_notify(it_sub->first, key, args...);
          ++it_sub;
        } else {
          it_sub = subscribers_container.erase(it_sub);
        }
      }
    }
  };

}  // namespace kagome::subscription

#endif  // KAGOME_SUBSCRIPTION_ENGINE_HPP
