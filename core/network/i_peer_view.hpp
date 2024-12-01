/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include <memory>
#include <unordered_map>

#include <libp2p/peer/peer_id.hpp>

#include "crypto/type_hasher.hpp"
#include "network/types/collator_messages.hpp"
#include "outcome/outcome.hpp"
#include "primitives/event_types.hpp"
#include "subscription/subscriber.hpp"
#include "subscription/subscription_engine.hpp"

namespace kagome::network {

  using HashedBlockHeader = primitives::BlockHeader;
  struct ExView {
    View view;
    HashedBlockHeader new_head;
    std::vector<primitives::BlockHash> lost;
  };

  struct ExViewRef {
    std::optional<std::reference_wrapper<const HashedBlockHeader>> new_head;
    const std::vector<primitives::BlockHash> &lost;
  };

  /**
   * Observable class for current heads and finalized block number tracking.
   */
  class IPeerView {
   public:
    enum struct EventType : uint32_t { kViewUpdated, kPeerRemoved };

    using PeerId = libp2p::peer::PeerId;

    using MyViewSubscriptionEngine = subscription::
        SubscriptionEngine<EventType, std::monostate, network::ExView>;
    using MyViewSubscriptionEnginePtr =
        std::shared_ptr<MyViewSubscriptionEngine>;
    using MyViewSubscriber = MyViewSubscriptionEngine::SubscriberType;
    using MyViewSubscriberPtr = std::shared_ptr<MyViewSubscriber>;

    using PeerViewSubscriptionEngine = subscription::
        SubscriptionEngine<EventType, std::monostate, PeerId, network::View>;
    using PeerViewSubscriptionEnginePtr =
        std::shared_ptr<PeerViewSubscriptionEngine>;
    using PeerViewSubscriber = PeerViewSubscriptionEngine::SubscriberType;
    using PeerViewSubscriberPtr = std::shared_ptr<PeerViewSubscriber>;

    virtual ~IPeerView() = default;

    virtual size_t peersCount() const = 0;
    virtual MyViewSubscriptionEnginePtr getMyViewObservable() = 0;
    virtual PeerViewSubscriptionEnginePtr getRemoteViewObservable() = 0;

    virtual void removePeer(const PeerId &peer_id) = 0;
    virtual void updateRemoteView(const PeerId &peer_id, network::View &&view) = 0;
    virtual const View &getMyView() const = 0;
  };

}  // namespace kagome::network
