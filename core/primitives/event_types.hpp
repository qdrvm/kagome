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
#include <libp2p/peer/peer_id.hpp>

#include "common/buffer.hpp"
#include "primitives/block_id.hpp"

#include "primitives/extrinsic.hpp"
#include "primitives/version.hpp"
#include "subscription/subscriber.hpp"
#include "subscription/subscription_engine.hpp"

namespace kagome::api {
  class Session;
}  // namespace kagome::api

namespace kagome::primitives {
  struct BlockHeader;
}

namespace kagome::primitives::events {

  enum struct ChainEventType : uint32_t {
    kNewHeads = 1,
    kFinalizedHeads = 2,
    kAllHeads = 3,
    kRuntimeVersion = 4
  };

  /**
   * - "future" - Transaction is part of the future queue.
   * - "ready" - Transaction is part of the ready queue.
   * - OBJECT - The transaction has been broadcast to the given peers.
   *     "broadcast": ARRAY
   *     STRING - PeerId.
   * - OBJECT - Transaction has been included in block with given hash.
   *     "inBlock": STRING - Hex-encoded hash of the block.
   * - OBJECT - "The block this transaction was included in has been retracted.
   *     "retracted": STRING - Hex-encoded hash of the block.
   * - OBJECT - Maximum number of finality watchers has been reached, old
   *    watches are being removed.
   *    "finalityTimeout": STRING - Hex-encoded hash of the block.
   * - OBJECT - Transaction has been finalized by GRANDPA.
   *    "finalized": STRING - Hex-encoded hash of the block.
   * - OBJECT - Transaction has been
   *    replaced in the pool, by another transaction that provides the same tags
   *    (e.g. same sender/nonce).
   *    "usurped": STRING - Hex-encoded hash of the transaction.
   * - "dropped" - Transaction has been dropped from the pool
   *    because of the limit.
   * - "invalid" - Transaction is no longer valid in the
   *    current state.
   */
  enum class ExtrinsicEventType {
    FUTURE,
    READY,
    BROADCAST,
    IN_BLOCK,
    RETRACTED,
    FINALITY_TIMEOUT,
    FINALIZED,
    USURPED,
    DROPPED,
    INVALID
  };

  struct BroadcastEventParams {
    std::vector<libp2p::peer::PeerId> peers;
  };

  struct InBlockEventParams {
    primitives::BlockHash block;
  };
  struct RetractedEventParams {
    primitives::BlockHash retracted_block;
  };

  struct FinalityTimeoutEventParams {
    primitives::BlockHash block;
  };

  struct FinalizedEventParams {
    primitives::BlockHash block;
  };

  struct UsurpedEventParams {
    common::Hash256 transaction_hash;
  };

  template <typename T>
  using ref = std::reference_wrapper<const T>;

  struct ExtrinsicLifecycleEvent {
    using Params = boost::variant<boost::none_t,
                                  BroadcastEventParams,
                                  InBlockEventParams,
                                  RetractedEventParams,
                                  FinalityTimeoutEventParams,
                                  FinalizedEventParams,
                                  UsurpedEventParams>;

    static ExtrinsicLifecycleEvent Future(ObservedExtrinsicId id) {
      return ExtrinsicLifecycleEvent{
          id, ExtrinsicEventType::FUTURE, Params{boost::none}};
    }

    static ExtrinsicLifecycleEvent Ready(ObservedExtrinsicId id) {
      return ExtrinsicLifecycleEvent{
          id, ExtrinsicEventType::READY, Params{boost::none}};
    }

    static ExtrinsicLifecycleEvent Broadcast(
        ObservedExtrinsicId id, std::vector<libp2p::peer::PeerId> peers) {
      return ExtrinsicLifecycleEvent{
          id,
          ExtrinsicEventType::BROADCAST,
          Params{BroadcastEventParams{.peers = std::move(peers)}}};
    }

    static ExtrinsicLifecycleEvent InBlock(ObservedExtrinsicId id,
                                           primitives::BlockHash block) {
      return ExtrinsicLifecycleEvent{
          id,
          ExtrinsicEventType::IN_BLOCK,
          Params{InBlockEventParams{.block = std::move(block)}}};
    }

    static ExtrinsicLifecycleEvent Retracted(
        ObservedExtrinsicId id, primitives::BlockHash retracted_block) {
      return ExtrinsicLifecycleEvent{
          id,
          ExtrinsicEventType::RETRACTED,
          Params{RetractedEventParams{.retracted_block = std::move(retracted_block)}}};
    }

    static ExtrinsicLifecycleEvent FinalityTimeout(
        ObservedExtrinsicId id, primitives::BlockHash block) {
      return ExtrinsicLifecycleEvent{
          id,
          ExtrinsicEventType::FINALITY_TIMEOUT,
          Params{FinalizedEventParams{.block = std::move(block)}}};
    }

    static ExtrinsicLifecycleEvent Finalized(ObservedExtrinsicId id,
                                             primitives::BlockHash block) {
      return ExtrinsicLifecycleEvent{
          id,
          ExtrinsicEventType::FINALIZED,
          Params{FinalizedEventParams{.block = std::move(block)}}};
    }

    static ExtrinsicLifecycleEvent Usurped(ObservedExtrinsicId id,
                                           common::Hash256 transaction_hash) {
      return ExtrinsicLifecycleEvent{
          id,
          ExtrinsicEventType::USURPED,
          Params{UsurpedEventParams{.transaction_hash = std::move(transaction_hash)}}};
    }

    static ExtrinsicLifecycleEvent Dropped(ObservedExtrinsicId id) {
      return ExtrinsicLifecycleEvent{
          id, ExtrinsicEventType::DROPPED, Params{boost::none}};
    }

    static ExtrinsicLifecycleEvent Invalid(ObservedExtrinsicId id) {
      return ExtrinsicLifecycleEvent{
          id, ExtrinsicEventType::INVALID, Params{boost::none}};
    }

    ObservedExtrinsicId id;
    ExtrinsicEventType type;

    boost::variant<boost::none_t,
                   BroadcastEventParams,
                   InBlockEventParams,
                   RetractedEventParams,
                   FinalityTimeoutEventParams,
                   FinalizedEventParams,
                   UsurpedEventParams>
        params;

   private:
    ExtrinsicLifecycleEvent(ObservedExtrinsicId id,
                            ExtrinsicEventType type,
                            Params params)
        : id{id}, type{type}, params{std::move(params)} {}
  };

  using ChainEventParams = boost::variant<boost::none_t,
                                          ref<const primitives::BlockHeader>,
                                          ref<primitives::Version>>;

  using StorageSubscriptionEngine =
      subscription::SubscriptionEngine<common::Buffer,
                                       std::shared_ptr<api::Session>,
                                       common::Buffer,
                                       primitives::BlockHash>;
  using StorageSubscriptionEnginePtr =
      std::shared_ptr<StorageSubscriptionEngine>;

  using StorageEventSubscriber = StorageSubscriptionEngine::SubscriberType;
  using StorageEventSubscriberPtr = std::shared_ptr<StorageEventSubscriber>;

  using ChainSubscriptionEngine =
      subscription::SubscriptionEngine<primitives::events::ChainEventType,
                                       std::shared_ptr<api::Session>,
                                       primitives::events::ChainEventParams>;
  using ChainSubscriptionEnginePtr = std::shared_ptr<ChainSubscriptionEngine>;

  using ChainEventSubscriber = ChainSubscriptionEngine::SubscriberType;
  using ChainEventSubscriberPtr = std::shared_ptr<ChainEventSubscriber>;

  using ExtrinsicSubscriptionEngine = subscription::SubscriptionEngine<
      primitives::events::ExtrinsicEventType,
      std::shared_ptr<api::Session>,
      primitives::events::ExtrinsicLifecycleEvent>;
  using ExtrinsicSubscriptionEnginePtr =
      std::shared_ptr<ExtrinsicSubscriptionEngine>;

  using ExtrinsicEventSubscriber = ExtrinsicSubscriptionEngine::SubscriberType;
  using ExtrinsicEventSubscriberPtr = std::shared_ptr<ExtrinsicEventSubscriber>;

}  // namespace kagome::primitives::events

#endif  // KAGOME_CORE_PRIMITIVES_EVENT_TYPES_HPP
