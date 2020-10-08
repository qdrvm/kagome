/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_SUBSCRIPTION_SUBSCRIBER_HPP
#define KAGOME_SUBSCRIPTION_SUBSCRIBER_HPP

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>

#include "subscription_engine.hpp"

namespace kagome::subscription {

  /**
   * Is a wrapper class, which provides subscription to events from
   * SubscriptionEngine
   * @tparam Key is a type of a subscription Key.
   * @tparam Type is a type of an object to receive notifications in.
   * @tparam Arguments is a set of types of objects needed to construct Type.
   */
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
    using SubscriptionSetId =
        typename SubscriptionEngineType::SubscriptionSetId;

    using CallbackFnType = std::function<void(
        SubscriptionSetId, ValueType &, const KeyType &, const Arguments &...)>;

   private:
    using SubscriptionsContainer =
        std::unordered_map<KeyType,
                           typename SubscriptionEngineType::IteratorType>;
    using SubscriptionsSets =
        std::unordered_map<SubscriptionSetId, SubscriptionsContainer>;

    std::atomic<SubscriptionSetId> next_id_;
    SubscriptionEnginePtr engine_;
    ValueType object_;

    std::mutex subscriptions_cs_;
    SubscriptionsSets subscriptions_sets_;

    CallbackFnType on_notify_callback_;

   public:
    template <typename... Args>
    explicit Subscriber(SubscriptionEnginePtr &ptr, Args &&... args)
        : next_id_(0ull), engine_(ptr), object_(std::forward<Args>(args)...) {}

    ~Subscriber() {
      /// Unsubscribe all
      for (auto &[_, subscriptions] : subscriptions_sets_)
        for (auto &[key, it] : subscriptions) engine_->unsubscribe(key, it);
    }

    Subscriber(const Subscriber &) = delete;
    Subscriber &operator=(const Subscriber &) = delete;

    Subscriber(Subscriber &&) = default;             // NOLINT
    Subscriber &operator=(Subscriber &&) = default;  // NOLINT

    void setCallback(CallbackFnType &&f) {
      on_notify_callback_ = std::move(f);
    }

    SubscriptionSetId generateSubscriptionSetId() {
      return ++next_id_;
    }

    void subscribe(SubscriptionSetId id, const KeyType &key) {
      std::lock_guard lock(subscriptions_cs_);
      auto &&[it, inserted] = subscriptions_sets_[id].emplace(
          key, typename SubscriptionEngineType::IteratorType{});

      /// Here we check first local subscriptions because of strong connection
      /// with SubscriptionEngine.
      if (inserted)
        it->second = engine_->subscribe(id, key, this->weak_from_this());
    }

    void unsubscribe(SubscriptionSetId id, const KeyType &key) {
      std::lock_guard<std::mutex> lock(subscriptions_cs_);
      if (auto set_it = subscriptions_sets_.find(id);
          set_it != subscriptions_sets_.end()) {
        auto &subscriptions = set_it->second;
        auto it = subscriptions.find(key);
        if (subscriptions.end() != it) {
          engine_->unsubscribe(key, it->second);
          subscriptions.erase(it);
        }
      }
    }

    void unsubscribe(SubscriptionSetId id) {
      std::lock_guard<std::mutex> lock(subscriptions_cs_);
      if (auto set_it = subscriptions_sets_.find(id);
          set_it != subscriptions_sets_.end()) {
        auto &subscriptions = set_it->second;
        for (auto &[key, it] : subscriptions) engine_->unsubscribe(key, it);

        subscriptions_sets_.erase(set_it);
      }
    }

    void unsubscribe() {
      std::lock_guard<std::mutex> lock(subscriptions_cs_);
      for (auto &[_, subscriptions] : subscriptions_sets_)
        for (auto &[key, it] : subscriptions) engine_->unsubscribe(key, it);

      subscriptions_sets_.clear();
    }

    void on_notify(SubscriptionSetId set_id,
                   const KeyType &key,
                   const Arguments &... args) {
      if (nullptr != on_notify_callback_)
        on_notify_callback_(set_id, object_, key, args...);
    }
  };

}  // namespace kagome::subscription

#endif  // KAGOME_SUBSCRIPTION_SUBSCRIBER_HPP
