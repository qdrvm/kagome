/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include <memory>
#include <unordered_map>

#include <libp2p/peer/peer_id.hpp>

#include "application/app_state_manager.hpp"
#include "blockchain/block_tree.hpp"
#include "crypto/type_hasher.hpp"
#include "injector/lazy.hpp"
#include "network/types/collator_messages.hpp"
#include "outcome/outcome.hpp"
#include "primitives/event_types.hpp"
#include "subscription/subscriber.hpp"
#include "subscription/subscription_engine.hpp"
#include "utils/non_copyable.hpp"
#include "utils/safe_object.hpp"

namespace kagome::blockchain {
  class BlockTree;
}

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
  class PeerView final : public NonCopyable,
                         public NonMovable,
                         public std::enable_shared_from_this<PeerView> {
   public:
    enum struct EventType : uint32_t { kViewUpdated, kPeerRemoved };

    using PeerId = libp2p::peer::PeerId;

    using MyViewSubscriptionEngine =
        subscription::SubscriptionEngine<EventType, bool, network::ExView>;
    using MyViewSubscriptionEnginePtr =
        std::shared_ptr<MyViewSubscriptionEngine>;
    using MyViewSubscriber = MyViewSubscriptionEngine::SubscriberType;
    using MyViewSubscriberPtr = std::shared_ptr<MyViewSubscriber>;

    using PeerViewSubscriptionEngine = subscription::
        SubscriptionEngine<EventType, bool, PeerId, network::View>;
    using PeerViewSubscriptionEnginePtr =
        std::shared_ptr<PeerViewSubscriptionEngine>;
    using PeerViewSubscriber = PeerViewSubscriptionEngine::SubscriberType;
    using PeerViewSubscriberPtr = std::shared_ptr<PeerViewSubscriber>;

    PeerView(primitives::events::ChainSubscriptionEnginePtr chain_events_engine,
             std::shared_ptr<application::AppStateManager> app_state_manager,
             LazySPtr<blockchain::BlockTree> block_tree);
    ~PeerView() = default;

    /**
     * Object lifetime control subsystem.
     */
    bool prepare();
    void stop();

    size_t peersCount() const;

    MyViewSubscriptionEnginePtr getMyViewObservable();
    PeerViewSubscriptionEnginePtr getRemoteViewObservable();

    void removePeer(const PeerId &peer_id);
    void updateRemoteView(const PeerId &peer_id, network::View &&view);
    std::optional<std::reference_wrapper<const ExView>> getMyView() const;

   private:
    void updateMyView(network::ExView &&view);

    primitives::events::ChainSub chain_sub_;

    MyViewSubscriptionEnginePtr my_view_update_observable_;
    PeerViewSubscriptionEnginePtr remote_view_update_observable_;

    std::optional<ExView> my_view_;
    SafeObject<std::unordered_map<PeerId, View>> remote_view_;
    LazySPtr<blockchain::BlockTree> block_tree_;
  };

}  // namespace kagome::network
