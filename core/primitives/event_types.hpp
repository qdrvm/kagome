/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_PRIMITIVES_EVENT_TYPES_HPP
#define KAGOME_CORE_PRIMITIVES_EVENT_TYPES_HPP

#include <cstdint>
#include <memory>

namespace kagome::api {
  class Session;
}  // kagome::api

namespace kagome::primitives {
  enum struct SubscriptionEventType : uint32_t {
    kNewHeads = 1
  };
}  // namespace kagome::primitives

namespace kagome::subscription {
  template <typename Key, typename Type, typename... Arguments>
  class SubscriptionEngine;

  template <typename Key, typename Type, typename... Arguments>
  class Subscriber;
}  // kagome::subscription

namespace kagome::subscriptions {
  using EventsSubscribedSessionType =
      subscription::Subscriber<primitives::SubscriptionEventType,
                               std::shared_ptr<api::Session>>;
  using EventsSubscribedSessionPtr =
      std::shared_ptr<EventsSubscribedSessionType>;

  using EventsSubscriptionEngineType =
      subscription::SubscriptionEngine<primitives::SubscriptionEventType,
                                       std::shared_ptr<api::Session>>;
  using EventsSubscriptionEnginePtr =
      std::shared_ptr<EventsSubscriptionEngineType>;
}  // kagome::subscriptions

#endif  // KAGOME_CORE_PRIMITIVES_EVENT_TYPES_HPP
