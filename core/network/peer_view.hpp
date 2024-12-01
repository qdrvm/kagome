/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include "network/i_peer_view.hpp"
#include "utils/non_copyable.hpp"
#include "utils/safe_object.hpp"
#include "injector/lazy.hpp"
#include "application/app_state_manager.hpp"
#include "blockchain/block_tree.hpp"

namespace kagome::blockchain {
  class BlockTree;
}

namespace kagome::network {

  /**
   * Observable class for current heads and finalized block number tracking.
   */
  class PeerView final : public NonCopyable,
                         public NonMovable,
                         public IPeerView,
                         public std::enable_shared_from_this<PeerView> {
   public:
    PeerView(primitives::events::ChainSubscriptionEnginePtr chain_events_engine,
             std::shared_ptr<application::AppStateManager> app_state_manager,
             LazySPtr<blockchain::BlockTree> block_tree);
    ~PeerView() = default;

    /**
     * Object lifetime control subsystem.
     */
    bool prepare();
    void stop();

    size_t peersCount() const override;
    MyViewSubscriptionEnginePtr getMyViewObservable() override;
    PeerViewSubscriptionEnginePtr getRemoteViewObservable() override;

    void removePeer(const PeerId &peer_id) override;
    void updateRemoteView(const PeerId &peer_id, network::View &&view) override;
    const View &getMyView() const override {
      return my_view_;
    }

   private:
    void updateMyView(const primitives::BlockHeader &header);

    primitives::events::ChainSub chain_sub_;
    LazySPtr<blockchain::BlockTree> block_tree_;

    MyViewSubscriptionEnginePtr my_view_update_observable_;
    PeerViewSubscriptionEnginePtr remote_view_update_observable_;

    View my_view_;
    SafeObject<std::unordered_map<PeerId, View>> remote_view_;
  };

}  // namespace kagome::network
