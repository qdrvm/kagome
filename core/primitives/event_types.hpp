/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <cstdint>
#include <memory>

#include <boost/none_t.hpp>
#include <boost/variant.hpp>
#include <libp2p/peer/peer_id.hpp>

#include "common/buffer.hpp"
#include "consensus/timeline/sync_state.hpp"
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

  template <typename T>
  using ref_t = std::reference_wrapper<const T>;

  enum struct ChainEventType : uint32_t {
    kNewHeads = 1,
    kFinalizedHeads = 2,
    kAllHeads = 3,
    kFinalizedRuntimeVersion = 4,
    kNewRuntime = 5,
    kDeactivateAfterFinalization = 6,
  };

  enum struct SyncStateEventType : uint32_t { kSyncState = 1 };

  using HeadsEventParams = ref_t<const primitives::BlockHeader>;
  using RuntimeVersionEventParams = ref_t<const primitives::Version>;
  using NewRuntimeEventParams = ref_t<const primitives::BlockHash>;
  using RemoveAfterFinalizationParams = std::vector<primitives::BlockHash>;

  using ChainEventParams = boost::variant<std::nullopt_t,
                                          HeadsEventParams,
                                          RuntimeVersionEventParams,
                                          NewRuntimeEventParams,
                                          RemoveAfterFinalizationParams>;

  using SyncStateEventParams = consensus::SyncState;

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

  using Hash256Span = std::span<const uint8_t, common::Hash256::size()>;

  struct BroadcastEventParams {
    std::span<const libp2p::peer::PeerId> peers;
  };

  struct InBlockEventParams {
    Hash256Span block;
  };
  struct RetractedEventParams {
    Hash256Span retracted_block;
  };

  struct FinalityTimeoutEventParams {
    Hash256Span block;
  };

  struct FinalizedEventParams {
    Hash256Span block;
  };

  struct UsurpedEventParams {
    Hash256Span transaction_hash;
  };

  /**
   * ID of an extrinsic being observed
   * @see autor_submitAndWatchExtrincis pubsub RPC call
   */
  using SubscribedExtrinsicId = uint32_t;

  struct ExtrinsicLifecycleEvent {
    // mind that although sometimes std::nullopt is used in variants to skip a
    // parameter, here it literally represents the type of event parameters when
    // there are none
    using Params = boost::variant<std::nullopt_t,
                                  BroadcastEventParams,
                                  InBlockEventParams,
                                  RetractedEventParams,
                                  FinalityTimeoutEventParams,
                                  FinalizedEventParams,
                                  UsurpedEventParams>;

    static ExtrinsicLifecycleEvent Future(SubscribedExtrinsicId id) {
      return ExtrinsicLifecycleEvent{
          id, ExtrinsicEventType::FUTURE, Params{std::nullopt}};
    }

    static ExtrinsicLifecycleEvent Ready(SubscribedExtrinsicId id) {
      return ExtrinsicLifecycleEvent{
          id, ExtrinsicEventType::READY, Params{std::nullopt}};
    }

    static ExtrinsicLifecycleEvent Broadcast(
        SubscribedExtrinsicId id, std::span<const libp2p::peer::PeerId> peers) {
      return ExtrinsicLifecycleEvent{
          id,
          ExtrinsicEventType::BROADCAST,
          Params{BroadcastEventParams{.peers = std::move(peers)}}};
    }

    static ExtrinsicLifecycleEvent InBlock(SubscribedExtrinsicId id,
                                           Hash256Span block) {
      return ExtrinsicLifecycleEvent{
          id,
          ExtrinsicEventType::IN_BLOCK,
          Params{InBlockEventParams{.block = std::move(block)}}};
    }

    static ExtrinsicLifecycleEvent Retracted(SubscribedExtrinsicId id,
                                             Hash256Span retracted_block) {
      return ExtrinsicLifecycleEvent{
          id,
          ExtrinsicEventType::RETRACTED,
          Params{RetractedEventParams{.retracted_block =
                                          std::move(retracted_block)}}};
    }

    static ExtrinsicLifecycleEvent FinalityTimeout(SubscribedExtrinsicId id,
                                                   Hash256Span block) {
      return ExtrinsicLifecycleEvent{
          id,
          ExtrinsicEventType::FINALITY_TIMEOUT,
          Params{FinalizedEventParams{.block = std::move(block)}}};
    }

    static ExtrinsicLifecycleEvent Finalized(SubscribedExtrinsicId id,
                                             Hash256Span block) {
      return ExtrinsicLifecycleEvent{
          id,
          ExtrinsicEventType::FINALIZED,
          Params{FinalizedEventParams{.block = std::move(block)}}};
    }

    static ExtrinsicLifecycleEvent Usurped(SubscribedExtrinsicId id,
                                           Hash256Span transaction_hash) {
      return ExtrinsicLifecycleEvent{
          id,
          ExtrinsicEventType::USURPED,
          Params{UsurpedEventParams{.transaction_hash =
                                        std::move(transaction_hash)}}};
    }

    static ExtrinsicLifecycleEvent Dropped(SubscribedExtrinsicId id) {
      return ExtrinsicLifecycleEvent{
          id, ExtrinsicEventType::DROPPED, Params{std::nullopt}};
    }

    static ExtrinsicLifecycleEvent Invalid(SubscribedExtrinsicId id) {
      return ExtrinsicLifecycleEvent{
          id, ExtrinsicEventType::INVALID, Params{std::nullopt}};
    }

    SubscribedExtrinsicId id;
    ExtrinsicEventType type;

    Params params;

   private:
    ExtrinsicLifecycleEvent(SubscribedExtrinsicId id,
                            ExtrinsicEventType type,
                            Params params)
        : id{id}, type{type}, params{std::move(params)} {}
  };

  // SubscriptionEngine for changes in trie storage
  using StorageSubscriptionEngine =
      subscription::SubscriptionEngine<common::Buffer,
                                       std::shared_ptr<api::Session>,
                                       std::optional<common::Buffer>,
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

  using SyncStateSubscriptionEngine = subscription::SubscriptionEngine<
      primitives::events::SyncStateEventType,
      bool,
      primitives::events::SyncStateEventParams>;
  using SyncStateSubscriptionEnginePtr =
      std::shared_ptr<SyncStateSubscriptionEngine>;

  using SyncStateEventSubscriber = SyncStateSubscriptionEngine::SubscriberType;
  using SyncStateEventSubscriberPtr = std::shared_ptr<SyncStateEventSubscriber>;

  using ExtrinsicSubscriptionEngine = subscription::SubscriptionEngine<
      SubscribedExtrinsicId,
      std::shared_ptr<api::Session>,
      primitives::events::ExtrinsicLifecycleEvent>;
  using ExtrinsicSubscriptionEnginePtr =
      std::shared_ptr<ExtrinsicSubscriptionEngine>;

  using ExtrinsicEventSubscriber = ExtrinsicSubscriptionEngine::SubscriberType;
  using ExtrinsicEventSubscriberPtr = std::shared_ptr<ExtrinsicEventSubscriber>;

  template <typename EventKey, typename Receiver, typename... Arguments>
  void subscribe(
      subscription::Subscriber<EventKey, Receiver, Arguments...> &sub,
      EventKey type,
      auto f) {
    sub.setCallback(
        [f{std::move(f)}](subscription::SubscriptionSetId,
                          Receiver &,
                          EventKey,
                          const Arguments &...args) { f(args...); });
    sub.subscribe(sub.generateSubscriptionSetId(), type);
  }

  struct ChainSub {
    ChainSub(ChainSubscriptionEnginePtr engine)
        : sub{std::make_shared<primitives::events::ChainEventSubscriber>(
            std::move(engine))} {}

    void onBlock(ChainEventType type, auto f) {
      subscribe(*sub, type, [f{std::move(f)}](const ChainEventParams &args) {
        auto &block = boost::get<HeadsEventParams>(args).get();
        if constexpr (std::is_invocable_v<decltype(f)>) {
          f();
        } else {
          f(block);
        }
      });
    }
    void onFinalize(auto f) {
      onBlock(ChainEventType::kFinalizedHeads, std::move(f));
    }
    void onHead(auto f) {
      onBlock(ChainEventType::kNewHeads, std::move(f));
    }
    void onDeactivate(auto f) {
      subscribe(*sub,
                ChainEventType::kDeactivateAfterFinalization,
                [f{std::move(f)}](const ChainEventParams &args) {
                  f(boost::get<RemoveAfterFinalizationParams>(args));
                });
    }

    ChainEventSubscriberPtr sub;
  };
}  // namespace kagome::primitives::events
