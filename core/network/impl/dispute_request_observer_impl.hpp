/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_NETWORK_DISPUTEREQUESTOBSERVERIMPL
#define KAGOME_NETWORK_DISPUTEREQUESTOBSERVERIMPL

#include "network/dispute_request_observer.hpp"

#include "primitives/authority_discovery_id.hpp"

namespace kagome::dispute {
  class DisputeCoordinator;
}

namespace kagome::network {

  class DisputeRequestObserverImpl final
      : public DisputeRequestObserver,
        public std::enable_shared_from_this<DisputeRequestObserverImpl> {
   public:
    using AuthorityDiscoveryId = primitives::AuthorityDiscoveryId;
    using Queue = std::deque<std::pair<DisputeMessage, CbOutcome>>;

    static constexpr size_t kPeerQueueCapacity = 10;

    explicit DisputeRequestObserverImpl(
        std::shared_ptr<dispute::DisputeCoordinator> dispute_coordinator);

    void onDisputeRequest(const DisputeMessage &request,
                          CbOutcome &&cb) override;

   private:
    /// Push an incoming request for a given authority.
    ///
    /// Returns: `Ok(())` if succeeded, `Err((args))` if capacity is reached.
    void push_req(AuthorityDiscoveryId peer,
                  const DisputeMessage &request,
                  CbOutcome &&cb);

    /// Pop all heads and return them for processing.
    ///
    /// This gets one message from each peer that has sent at least one.
    ///
    /// This function is rate limited, if called in sequence it will not
    /// return more often than every `RECEIVE_RATE_LIMIT`.
    ///
    /// NOTE: If empty this function will not return `Ready` at all, but
    /// will always be `Pending`.
    std::vector<std::pair<DisputeMessage, DisputeRequestObserver::CbOutcome>>
    pop_reqs();

    std::shared_ptr<dispute::DisputeCoordinator> dispute_coordinator_;

    /// Queues for messages from authority peers for rate limiting.
    ///
    /// Invariants ensured:
    ///
    /// 1. No queue will ever have more than `PEER_QUEUE_CAPACITY` elements.
    /// 2. There are no empty queues. Whenever a queue gets empty, it is
    ///    removed. This way checking whether there are any messages queued is
    ///    cheap.
    /// 3. As long as not empty, `pop_reqs` will, if called in sequence, not
    ///    return `Ready` more often than once for every `RECEIVE_RATE_LIMIT`,
    ///    but it will always return Ready eventually.
    /// 4. If empty `pop_reqs` will never return `Ready`, but will always be
    ///    `Pending`.

    /// Actual queues.
    std::unordered_map<AuthorityDiscoveryId, Queue> queues_;

    /// Delay timer for establishing the rate limit.
    auto t = std::make_unique<clock::BasicWaitableTimer>(
        internal_context_->io_context());

    std::optional<uint32_t> rate_limit_timer_;
  };

}  // namespace kagome::network

#endif  // KAGOME_NETWORK_DISPUTEREQUESTOBSERVERIMPL
