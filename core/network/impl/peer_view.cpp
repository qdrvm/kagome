/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "network/peer_view.hpp"
#include "blockchain/block_tree.hpp"
#include "common/visitor.hpp"

namespace kagome::network {

  PeerView::PeerView(
      const primitives::events::ChainSubscriptionEnginePtr &chain_events_engine,
      std::shared_ptr<application::AppStateManager> asmgr,
      std::shared_ptr<blockchain::BlockTree> block_tree)
      : chain_events_engine_{chain_events_engine},
        my_view_update_observable_{
            std::make_shared<MyViewSubscriptionEngine>()},
        remote_view_update_observable_{
            std::make_shared<PeerViewSubscriptionEngine>()},
        block_tree_{std::move(block_tree)} {
    BOOST_ASSERT(chain_events_engine_);
    BOOST_ASSERT(block_tree_);
    asmgr->takeControl(*this);
  }

  bool PeerView::start() {
    return true;
  }

  void PeerView::stop() {
    if (chain_sub_) {
      chain_sub_->unsubscribe();
    }
    chain_sub_.reset();
  }

  bool PeerView::prepare() {
    chain_sub_ = std::make_shared<primitives::events::ChainEventSubscriber>(
        chain_events_engine_);
    chain_sub_->subscribe(chain_sub_->generateSubscriptionSetId(),
                          primitives::events::ChainEventType::kNewHeads);
    chain_sub_->setCallback(
        [wptr{weak_from_this()}](
            auto /*set_id*/,
            auto && /*internal_obj*/,
            auto /*event_type*/,
            const primitives::events::ChainEventParams &event) {
          if (auto self = wptr.lock()) {
            if (auto const value =
                    if_type<const primitives::events::HeadsEventParams>(
                        event)) {
              self->updateMyView(
                  View{.heads_ = self->block_tree_->getLeaves(),
                       .finalized_number_ =
                           self->block_tree_->getLastFinalized().number});
            }
          }
        });
    return true;
  }

  PeerView::MyViewSubscriptionEnginePtr PeerView::getMyViewObservable() {
    BOOST_ASSERT(my_view_update_observable_);
    return my_view_update_observable_;
  }

  PeerView::PeerViewSubscriptionEnginePtr PeerView::getRemoteViewObservable() {
    BOOST_ASSERT(remote_view_update_observable_);
    return remote_view_update_observable_;
  }

  void PeerView::updateMyView(network::View &&view) {
    BOOST_ASSERT(my_view_update_observable_);
    std::sort(view.heads_.begin(), view.heads_.end());
    if (my_view_ != view) {
      my_view_ = std::move(view);

      BOOST_ASSERT(my_view_);
      my_view_update_observable_->notify(EventType::kViewUpdated, *my_view_);
    }
  }

  void PeerView::updateRemoteView(const PeerId &peer_id, network::View &&view) {
    auto it = remote_view_.find(peer_id);
    if (it == remote_view_.end() || it->second != view) {
      auto &ref = remote_view_[peer_id];
      ref = std::move(view);
      remote_view_update_observable_->notify(
          EventType::kViewUpdated, peer_id, ref);
    }
  }

  std::optional<std::reference_wrapper<const View>> PeerView::getMyView()
      const {
    return my_view_;
  }

}  // namespace kagome::network
