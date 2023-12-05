/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "network/peer_view.hpp"
#include "blockchain/block_tree.hpp"
#include "common/visitor.hpp"

namespace kagome::network {

  PeerView::PeerView(
      primitives::events::ChainSubscriptionEnginePtr chain_events_engine,
      std::shared_ptr<application::AppStateManager> app_state_manager,
      LazySPtr<blockchain::BlockTree> block_tree)
      : chain_events_engine_{chain_events_engine},
        my_view_update_observable_{
            std::make_shared<MyViewSubscriptionEngine>()},
        remote_view_update_observable_{
            std::make_shared<PeerViewSubscriptionEngine>()},
        block_tree_(std::move(block_tree)) {
    BOOST_ASSERT(chain_events_engine_);
    app_state_manager->takeControl(*this);
  }

  void PeerView::stop() {
    if (chain_sub_) {
      chain_sub_->unsubscribe();
    }
    chain_sub_.reset();
  }

  primitives::events::ChainSubscriptionEnginePtr
  PeerView::intoChainEventsEngine() {
    return chain_events_engine_;
  }

  bool PeerView::prepare() {
    chain_sub_ = std::make_shared<primitives::events::ChainEventSubscriber>(
        chain_events_engine_);
    chain_sub_->subscribe(chain_sub_->generateSubscriptionSetId(),
                          primitives::events::ChainEventType::kNewHeads);
    chain_sub_->setCallback([wptr{weak_from_this()}](
                                auto /*set_id*/,
                                auto && /*internal_obj*/,
                                auto /*event_type*/,
                                const primitives::events::ChainEventParams
                                    &event) {
      if (auto self = wptr.lock()) {
        if (auto const value =
                if_type<const primitives::events::HeadsEventParams>(event)) {
          self->updateMyView(ExView{
              .view =
                  View{
                      .heads_ = self->block_tree_.get()->getLeaves(),
                      .finalized_number_ =
                          self->block_tree_.get()->getLastFinalized().number,
                  },
              .new_head = (*value).get()});
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

  void PeerView::updateMyView(network::ExView &&view) {
    BOOST_ASSERT(my_view_update_observable_);
    std::sort(view.view.heads_.begin(), view.view.heads_.end());
    if (!my_view_ || my_view_->view != view.view
        || my_view_->new_head != view.new_head) {
      if (my_view_) {
        view.lost.swap(my_view_->lost);
        view.lost.clear();
        for (const auto &head : my_view_->view.heads_) {
          if (!view.view.contains(head)) {
            view.lost.emplace_back(head);
          }
        }
      }
      my_view_ = std::move(view);

      BOOST_ASSERT(my_view_);
      my_view_update_observable_->notify(EventType::kViewUpdated, *my_view_);
    }
  }

  void PeerView::removePeer(const PeerId &peer_id) {
    if (auto it = remote_view_.find(peer_id); it != remote_view_.end()) {
      network::View old_view{std::move(it->second)};
      remote_view_.erase(peer_id);
      remote_view_update_observable_->notify(
          EventType::kPeerRemoved, peer_id, std::move(old_view));
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

  std::optional<std::reference_wrapper<const ExView>> PeerView::getMyView()
      const {
    return my_view_;
  }

}  // namespace kagome::network
