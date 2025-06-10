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
#include <libp2p/common/shared_fn.hpp>
#include <libp2p/peer/peer_id.hpp>
#include <qtils/option_take.hpp>

#include "common/buffer.hpp"
#include "consensus/timeline/sync_state.hpp"
#include "primitives/block_id.hpp"
#include "primitives/extrinsic.hpp"
#include "primitives/version.hpp"
#include "storage/trie/types.hpp"
#include "subscription/subscriber.hpp"
#include "subscription/subscription_engine.hpp"

namespace kagome::api {
  class Session;
}  // namespace kagome::api

namespace kagome::primitives {
  struct BlockHeader;
}

namespace kagome::storage::trie {
  class PolkadotTrie;
}

namespace kagome::primitives::events {

  template <typename T>
  using ref_t = std::reference_wrapper<const T>;

  // TODO(Harrm) for review: why Heads? Is it short for Headers? It looks odd.
  enum struct ChainEventType : uint8_t {
    kNewHeads = 1,
    kFinalizedHeads = 2,
    kAllHeads = 3,
    kFinalizedRuntimeVersion = 4,
    kNewRuntime = 5,
    kDeactivateAfterFinalization = 6,  // TODO(kamilsa): #2369 might not be
                                       // triggered on every leaf deactivated
    kDiscardedHeads = 7,
    kNewStateSynced = 8,  // TODO(Harrm) it's arguable that it belongs here but
                          // it's very convenient for Direct Storage
  };

  enum struct PeerEventType : uint8_t {
    kConnected = 1,
    kDisconnected = 2,
  };

  enum struct SyncStateEventType : uint8_t { kSyncState = 1 };

  using HeadsEventParams = ref_t<const primitives::BlockHeader>;
  using RuntimeVersionEventParams = ref_t<const primitives::Version>;
  using NewRuntimeEventParams = ref_t<const primitives::BlockHash>;
  struct RemoveAfterFinalizationParams {
    struct HeaderInfo {
      primitives::BlockHash hash;
      primitives::BlockNumber number{};
    };
    std::vector<HeaderInfo> removed;
    primitives::BlockNumber finalized{};
  };
  struct NewStateSyncedParams {
    storage::trie::RootHash state_root;
    const storage::trie::PolkadotTrie &trie;
  };

  using ChainEventParams = std::variant<std::nullopt_t,
                                        HeadsEventParams,
                                        RuntimeVersionEventParams,
                                        NewRuntimeEventParams,
                                        RemoveAfterFinalizationParams,
                                        NewStateSyncedParams>;

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
  enum class ExtrinsicEventType : uint8_t {
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
          Params{BroadcastEventParams{.peers = peers}}};
    }

    static ExtrinsicLifecycleEvent InBlock(SubscribedExtrinsicId id,
                                           Hash256Span block) {
      return ExtrinsicLifecycleEvent{
          id,
          ExtrinsicEventType::IN_BLOCK,
          Params{InBlockEventParams{.block = block}}};
    }

    static ExtrinsicLifecycleEvent Retracted(SubscribedExtrinsicId id,
                                             Hash256Span retracted_block) {
      return ExtrinsicLifecycleEvent{
          id,
          ExtrinsicEventType::RETRACTED,
          Params{RetractedEventParams{.retracted_block = retracted_block}}};
    }

    static ExtrinsicLifecycleEvent FinalityTimeout(SubscribedExtrinsicId id,
                                                   Hash256Span block) {
      return ExtrinsicLifecycleEvent{
          id,
          ExtrinsicEventType::FINALITY_TIMEOUT,
          Params{FinalizedEventParams{.block = block}}};
    }

    static ExtrinsicLifecycleEvent Finalized(SubscribedExtrinsicId id,
                                             Hash256Span block) {
      return ExtrinsicLifecycleEvent{
          id,
          ExtrinsicEventType::FINALIZED,
          Params{FinalizedEventParams{.block = block}}};
    }

    static ExtrinsicLifecycleEvent Usurped(SubscribedExtrinsicId id,
                                           Hash256Span transaction_hash) {
      return ExtrinsicLifecycleEvent{
          id,
          ExtrinsicEventType::USURPED,
          Params{UsurpedEventParams{.transaction_hash = transaction_hash}}};
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

  using PeerSubscriptionEngine =
      subscription::SubscriptionEngine<primitives::events::PeerEventType,
                                       std::monostate,
                                       libp2p::peer::PeerId>;
  using PeerSubscriptionEnginePtr = std::shared_ptr<PeerSubscriptionEngine>;
  using PeerEventSubscriber = PeerSubscriptionEngine::SubscriberType;
  using PeerEventSubscriberPtr = std::shared_ptr<PeerEventSubscriber>;

  using ChainSubscriptionEngine =
      subscription::SubscriptionEngine<primitives::events::ChainEventType,
                                       std::shared_ptr<api::Session>,
                                       primitives::events::ChainEventParams>;
  using ChainSubscriptionEnginePtr = std::shared_ptr<ChainSubscriptionEngine>;
  using ChainEventSubscriber = ChainSubscriptionEngine::SubscriberType;
  using ChainEventSubscriberPtr = std::shared_ptr<ChainEventSubscriber>;

  using SyncStateSubscriptionEngine = subscription::SubscriptionEngine<
      primitives::events::SyncStateEventType,
      std::monostate,
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
                          const Arguments &...args) mutable { f(args...); });
    sub.subscribe(sub.generateSubscriptionSetId(), type);
  }

  template <typename EventKey, typename Receiver, typename... Arguments>
  auto subscribe(
      std::shared_ptr<
          subscription::SubscriptionEngine<EventKey, Receiver, Arguments...>>
          engine,
      EventKey type,
      auto f) {
    auto sub = std::make_shared<
        typename decltype(engine)::element_type::SubscriberType>(
        std::move(engine));
    subscribe(*sub, type, std::move(f));
    return sub;
  }

  struct ChainSub {
    ChainSub(ChainSubscriptionEnginePtr engine)
        : sub{std::make_shared<primitives::events::ChainEventSubscriber>(
              std::move(engine))} {}

    void onBlock(ChainEventType type, auto f) {
      subscribe(*sub, type, [f{std::move(f)}](const ChainEventParams &args) {
        auto &block = std::get<HeadsEventParams>(args).get();
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

    // TODO(kamilsa): #2369 not all deactivated leaves end up in this event
    void onDeactivate(auto f) {
      subscribe(*sub,
                ChainEventType::kDeactivateAfterFinalization,
                [f{std::move(f)}](const ChainEventParams &args) {
                  f(std::get<RemoveAfterFinalizationParams>(args));
                });
    }

    ChainEventSubscriberPtr sub;
  };

  std::shared_ptr<void> onSync(SyncStateSubscriptionEnginePtr engine, auto f) {
    return subscribe(
        std::move(engine),
        SyncStateEventType::kSyncState,
        libp2p::SharedFn{[f_{std::make_optional(std::move(f))}](
                             const SyncStateEventParams &event) mutable {
          if (event == consensus::SyncState::SYNCHRONIZED) {
            if (auto f = qtils::optionTake(f_)) {
              (*f)();
            }
          }
        }});
  }
}  // namespace kagome::primitives::events
