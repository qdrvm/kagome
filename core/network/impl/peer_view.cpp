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
      : chain_sub_{std::move(chain_events_engine)},
        my_view_update_observable_{
            std::make_shared<MyViewSubscriptionEngine>()},
        remote_view_update_observable_{
            std::make_shared<PeerViewSubscriptionEngine>()},
        block_tree_(std::move(block_tree)) {
    app_state_manager->takeControl(*this);
  }

  void PeerView::stop() {
    chain_sub_.sub->unsubscribe();
  }

  bool PeerView::prepare() {
    chain_sub_.onHead(
        [weak{weak_from_this()}](const primitives::BlockHeader &header) {
          if (auto self = weak.lock()) {
            self->updateMyView(ExView{
                .view =
                    View{
                        .heads_ = self->block_tree_.get()->getLeaves(),
                        .finalized_number_ =
                            self->block_tree_.get()->getLastFinalized().number,
                    },
                .new_head = header,
                .lost = {},
            });
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

  size_t PeerView::peersCount() const {
    return remote_view_.sharedAccess([](const auto &rv) { return rv.size(); });
  }

  void PeerView::removePeer(const PeerId &peer_id) {
    auto ref = remote_view_.exclusiveAccess(
        [&](auto &rv) -> std::optional<network::View> {
          if (auto it = rv.find(peer_id); it != rv.end()) {
            network::View old_view{std::move(it->second)};
            rv.erase(peer_id);
            return old_view;
          }
          return std::nullopt;
        });

    if (ref) {
      remote_view_update_observable_->notify(
          EventType::kPeerRemoved, peer_id, std::move(*ref));
    }
  }

  void PeerView::updateRemoteView(const PeerId &peer_id, network::View &&view) {
    auto ref = remote_view_.exclusiveAccess(
        [&](auto &rv) -> std::optional<network::View> {
          auto it = rv.find(peer_id);
          if (it == rv.end() || it->second != view) {
            auto &ref = rv[peer_id];
            ref = std::move(view);
            return ref;
          }
          return std::nullopt;
        });

    if (ref) {
      remote_view_update_observable_->notify(
          EventType::kViewUpdated, peer_id, *ref);
    }
  }

  std::optional<std::reference_wrapper<const ExView>> PeerView::getMyView()
      const {
    return my_view_;
  }

}  // namespace kagome::network
