/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef KAGOME_PEER_VIEW
#define KAGOME_PEER_VIEW

#include <libp2p/peer/peer_id.hpp>
#include <memory>
#include <unordered_map>

#include "application/app_state_manager.hpp"
#include "network/types/collator_messages.hpp"
#include "outcome/outcome.hpp"
#include "primitives/event_types.hpp"
#include "subscription/subscriber.hpp"
#include "subscription/subscription_engine.hpp"
#include "utils/non_copyable.hpp"

namespace kagome::network {

  class PeerView : public NonCopyable,
                   public NonMovable,
                   public std::enable_shared_from_this<PeerView> {
   public:
    enum struct EventType : uint32_t { kViewUpdated };

    using PeerId = libp2p::peer::PeerId;

    using MyViewSubscriptionEngine =
        subscription::SubscriptionEngine<EventType, bool, network::View>;
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

    PeerView(const primitives::events::ChainSubscriptionEnginePtr
                 &chain_events_engine,
             std::shared_ptr<application::AppStateManager> asmgr);
    virtual ~PeerView() = default;

    /**
     * Object lifetime control subsystem.
     */
    bool start();
    void stop();
    bool prepare();

    MyViewSubscriptionEnginePtr getMyViewObservable();
    PeerViewSubscriptionEnginePtr getRemoteViewObservable();

    void updateRemoteView(const PeerId &peer_id, network::View &&view);
    std::optional<std::reference_wrapper<const View>> getMyView() const;

   private:
    void updateMyView(network::View &&view);

    primitives::events::ChainSubscriptionEnginePtr chain_events_engine_;
    std::shared_ptr<primitives::events::ChainEventSubscriber> chain_sub_;

    MyViewSubscriptionEnginePtr my_view_update_observable_;
    PeerViewSubscriptionEnginePtr remote_view_update_observable_;

    std::optional<View> my_view_;
    std::unordered_map<PeerId, View> remote_view_;
  };

}  // namespace kagome::network

#endif  // KAGOME_PEER_VIEW
