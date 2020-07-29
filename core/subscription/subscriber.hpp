/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_SUBSCRIBER_HPP
#define KAGOME_SUBSCRIBER_HPP

#include <functional>
#include <memory>
#include <mutex>

#include "subscription_engine.hpp"

namespace kagome::subscription {

  template <typename Key, typename Type, typename... Arguments>
  class Subscriber final : public std::enable_shared_from_this<
                               Subscriber<Key, Type, Arguments...>> {
   public:
    using KeyType = Key;
    using ValueType = Type;
    using HashType = size_t;

    using SubscriptionEngineType =
        SubscriptionEngine<KeyType, ValueType, Arguments...>;
    using SubscriptionEnginePtr = std::shared_ptr<SubscriptionEngineType>;

   private:
    using SubscriptionsContainer =
        std::unordered_map<KeyType,
                           typename SubscriptionEngineType::IteratorType>;

    SubscriptionEnginePtr engine_;
    ValueType object_;

    std::mutex subscriptions_cs_;
    SubscriptionsContainer subscriptions_;

    std::function<void(Arguments const &...)> on_notify_callback_;

   public:
    template <typename... Args>
    explicit Subscriber(SubscriptionEnginePtr &ptr, Args &&... args)
        : engine_(ptr), object_(std::forward<Args>(args)...) {}

    ~Subscriber() {
      /// Unsubscribe all
      for (auto &[key, it] : subscriptions_) engine_->unsubscribe(key, it);
    }

    Subscriber(Subscriber const &) = delete;
    Subscriber &operator=(Subscriber const &) = delete;

    Subscriber(Subscriber &&) = default;
    Subscriber &operator=(Subscriber &&) = default;

    void set_callback(std::function<void(Arguments const &...)> &&f) {
      on_notify_callback_ = std::move(f);
    }

    void subscribe(KeyType const &key) {
      std::lock_guard lock(subscriptions_cs_);
      auto &&[it, inserted] = subscriptions_.emplace(key, typename SubscriptionEngineType::IteratorType{});

      /// Here we check first local subscriptions because of strong connection
      /// with SubscriptionEngine.
      if (inserted)
        it->second = engine_->subscribe(key, this->weak_from_this());
    }

    void unsubscribe(KeyType const &key) {
      std::lock_guard<std::mutex> lock(subscriptions_cs_);
      auto it = subscriptions_.find(key);
      if (subscriptions_.end() != it) {
        engine_->unsubscribe(key, it->second);
        subscriptions_.erase(it);
      }
    }

    void on_notify(Arguments const &... args) {
      if (nullptr != on_notify_callback_) on_notify_callback_(args...);
    }
  };

}  // namespace kagome::subscription

#endif  // KAGOME_SUBSCRIBER_HPP
