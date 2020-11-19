/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_PRIMITIVES_EVENT_TYPES_HPP
#define KAGOME_CORE_PRIMITIVES_EVENT_TYPES_HPP

#include <cstdint>
#include <memory>

#include <boost/none_t.hpp>
#include <boost/variant.hpp>

#include "common/buffer.hpp"
#include "primitives/block_id.hpp"
#include "primitives/version.hpp"
#include "subscription/subscriber.hpp"
#include "subscription/subscription_engine.hpp"

namespace kagome::api {
  class Session;
}  // namespace kagome::api

namespace kagome::primitives {
  struct BlockHeader;

  enum struct SubscriptionEventType : uint32_t {
    kNewHeads = 1,
    kFinalizedHeads = 2,
    kAllHeads = 3,
    kRuntimeVersion = 4
  };
}  // namespace kagome::primitives

namespace kagome::subscription {
  template <typename Key, typename Type, typename... Arguments>
  class SubscriptionEngine;

  template <typename Key, typename Type, typename... Arguments>
  class Subscriber;
}  // namespace kagome::subscription

namespace kagome::subscriptions {
  using EventsSubscriptionEngineType = subscription::SubscriptionEngine<
      primitives::SubscriptionEventType,
      std::shared_ptr<api::Session>,
      boost::variant<std::reference_wrapper<boost::none_t>,
                     std::reference_wrapper<const primitives::BlockHeader>,
                     std::reference_wrapper<primitives::Version>>>;
  using EventsSubscriptionEnginePtr =
      std::shared_ptr<EventsSubscriptionEngineType>;
  using EventsSubscribedSessionType =
      EventsSubscriptionEngineType::SubscriberType;
  using EventsSubscribedSessionPtr =
      std::shared_ptr<EventsSubscribedSessionType>;

  using SubscriptionEngineType =
      subscription::SubscriptionEngine<common::Buffer,
                                       std::shared_ptr<api::Session>,
                                       common::Buffer,
                                       primitives::BlockHash>;
  using SubscriptionEnginePtr = std::shared_ptr<SubscriptionEngineType>;

  using SubscribedSessionType = SubscriptionEngineType::SubscriberType;
  using SubscribedSessionPtr = std::shared_ptr<SubscribedSessionType>;

}  // namespace kagome::subscriptions

#endif  // KAGOME_CORE_PRIMITIVES_EVENT_TYPES_HPP
