/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "network/peer_view.hpp"
#include "blockchain/block_tree.hpp"
#include "common/visitor.hpp"
#include "utils/weak_macro.hpp"

namespace kagome::network {
  static constexpr size_t MAX_VIEW_HEADS = 5;

  inline std::pair<View, std::vector<primitives::BlockHash>> makeView(
      const LazySPtr<blockchain::BlockTree> &block_tree) {
    auto last_finalized = block_tree.get()->getLastFinalized().number;
    const auto leaves = block_tree.get()->getLeavesInfo();

    std::vector<std::pair<BlockNumber, primitives::BlockHash>> heads;
    for (const auto &bi : block_tree.get()->getLeavesInfo()) {
      if (bi.number >= last_finalized) {
        heads.emplace_back(bi.number, bi.hash);
      }
    }
    std::ranges::sort(
        heads, [](const auto &l, const auto &r) { return l.first < r.first; });

    std::vector<primitives::BlockHash> heads_;
    heads_.reserve(std::min(MAX_VIEW_HEADS, heads.size()));
    for (auto it = heads.rbegin(); it != heads.rend(); ++it) {
      if (heads_.size() < MAX_VIEW_HEADS) {
        heads_.emplace_back(it->second);
      } else {
        break;
      }
    }
    std::ranges::sort(heads_);
    assert(heads_.size() <= MAX_VIEW_HEADS);

    View view{
        .heads_ = {},
        .finalized_number_ = last_finalized,
    };
    std::ranges::transform(heads,
                           std::back_inserter(view.heads_),
                           [](const auto &data) { return data.second; });

    std::ranges::sort(view.heads_);
    return {view, heads_};
  }

  PeerView::PeerView(
      primitives::events::ChainSubscriptionEnginePtr chain_events_engine,
      std::shared_ptr<application::AppStateManager> app_state_manager,
      LazySPtr<blockchain::BlockTree> block_tree)
      : chain_sub_{std::move(chain_events_engine)},
        block_tree_{std::move(block_tree)},
        my_view_update_observable_{
            std::make_shared<MyViewSubscriptionEngine>()},
        remote_view_update_observable_{
            std::make_shared<PeerViewSubscriptionEngine>()},
        my_view_{makeView(block_tree_).first} {
    app_state_manager->takeControl(*this);
  }

  void PeerView::stop() {
    chain_sub_.sub->unsubscribe();
  }

  bool PeerView::prepare() {
    chain_sub_.onHead([WEAK_SELF](const primitives::BlockHeader &header) {
      WEAK_LOCK(self);
      self->updateMyView(header);
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

  void PeerView::updateMyView(const primitives::BlockHeader &header) {
    BOOST_ASSERT(my_view_update_observable_);
    auto [view, stripped_view] = makeView(block_tree_);
    const auto last_finalized = view.finalized_number_;
    ExView event{
        .view = std::move(view),
        .stripped_view =
            View{
                .heads_ = std::move(stripped_view),
                .finalized_number_ = last_finalized,
            },
        .new_head = header,
        .lost = {},
    };
    if (event.view == my_view_) {
      return;
    }
    for (const auto &head : my_view_.heads_) {
      if (not event.view.contains(head)) {
        event.lost.emplace_back(head);
      }
    }
    my_view_ = event.view;
    my_view_update_observable_->notify(EventType::kViewUpdated, event);
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
          EventType::kPeerRemoved, peer_id, *ref);
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
}  // namespace kagome::network
