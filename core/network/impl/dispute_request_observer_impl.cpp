/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "network/impl/dispute_request_observer_impl.hpp"

#include "dispute_coordinator/dispute_coordinator.hpp"
#include "network/types/dispute_messages.hpp"

namespace kagome::network {

  DisputeRequestObserverImpl::DisputeRequestObserverImpl(
      std::shared_ptr<kagome::dispute::DisputeCoordinator> dispute_coordinator)
      : dispute_coordinator_(std::move(dispute_coordinator)) {
    BOOST_ASSERT(dispute_coordinator_ != nullptr);
  }

  void DisputeRequestObserverImpl::onDisputeRequest(
      const DisputeMessage &request, DisputeRequestObserver::CbOutcome &&cb) {
    dispute_coordinator_->handle_incoming_ImportStatements();

    // TODO(xDimon): Implementation needed
    throw std::runtime_error("Not implemented yet");
  }

  void DisputeRequestObserverImpl::push_req(
      DisputeRequestObserverImpl::AuthorityDiscoveryId peer,
      const DisputeMessage &request,
      DisputeRequestObserver::CbOutcome &&cb) {
    auto &queue = queues_[peer];

    if (queue.size() >= kPeerQueueCapacity) {
      // TODO Err  return Err((occupied.key().clone(), req))
    }
    queue.emplace_back(request, std::move(cb));

    // We have at least one element to process - rate limit `timer` needs to
    // exist now:
    ensure_timer();
  }

  std::vector<DisputeMessage, DisputeRequestObserver::CbOutcome>
  DisputeRequestObserverImpl::pop_reqs() {
    wait_for_timer().await;

    std::vector<DisputeMessage, DisputeRequestObserver::CbOutcome> heads;
    heads.reserve(queues_.size());

    auto old_queues = std::move(queues_);

    for (auto &[peer, queue] : old_queues) {
      BOOST_ASSERT_MSG(not queue.empty(),
                       "Invariant that queues are never empty is broken.");

      heads.push(std::move(queue.front()));
      queue.pop_front();

      if (not queue.empty()) {
        queues_.emplace(peer, queue);
      }
    }

    if (not queues_.empty()) {
      // Still not empty - we should get woken at some point.
      ensure_timer();
    }

    return heads;
  }

}  // namespace kagome::network
