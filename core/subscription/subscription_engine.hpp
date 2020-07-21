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

    /// List preferable here because of remains the whole containers iterators
    /// alive after remove from the middle
    /// TODO(iceseer): remove processor cache penalty, while iterating, using
    /// custom allocator
    using SubscribersContainer = std::list<SubscriberWPtr>;
    using IteratorType = typename SubscribersContainer::iterator;

   private:
    template <typename KeyType, typename ValueType, typename... Args>
    friend class Subscriber;
    using KeyValueContainer = std::unordered_map<KeyType, SubscribersContainer>;

    std::shared_mutex subscribers_map_cs_;
    KeyValueContainer subscribers_map_;

   public:
    SubscriptionEngine() = default;
    ~SubscriptionEngine() = default;

    SubscriptionEngine(SubscriptionEngine &&) noexcept = default;
    SubscriptionEngine &operator=(SubscriptionEngine &&) noexcept = default;

    SubscriptionEngine(SubscriptionEngine const &) = delete;
    SubscriptionEngine &operator=(SubscriptionEngine const &) = delete;

   private:
    IteratorType subscribe(KeyType const &key, SubscriberWPtr ptr) {
      std::unique_lock lock(subscribers_map_cs_);
      auto &subscribers_list = subscribers_map_[key];
      return subscribers_list.emplace(subscribers_list.end(), std::move(ptr));
    }

    void unsubscribe(KeyType const &key, IteratorType const &it_remove) {
      std::unique_lock lock(subscribers_map_cs_);
      auto it = subscribers_map_.find(key);
      if (subscribers_map_.end() != it) {
        it->second.erase(it_remove);
        if (it->second.empty()) subscribers_map_.erase(it);
      }
    }

   public:
    void notify(KeyType const &key, Arguments const &... args) {
      std::shared_lock lock(subscribers_map_cs_);
      auto it = subscribers_map_.find(key);
      if (subscribers_map_.end() == it) return;

      auto &subscribers_container = it->second;
      for (auto it_sub = subscribers_container.begin();
           it_sub != subscribers_container.end();) {
        if (auto sub = it_sub->lock()) {
          sub->on_notify(args...);
          ++it_sub;
        } else {
          it_sub = subscribers_container.erase(it_sub);
        }
      }
    }
  };

}  // namespace kagome::subscription

#endif  // KAGOME_SUBSCRIPTION_ENGINE_HPP
