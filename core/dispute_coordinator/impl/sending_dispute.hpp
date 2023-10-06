/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_DISPUTE_SENDINGDISPUTE
#define KAGOME_DISPUTE_SENDINGDISPUTE

#include "dispute_coordinator/dispute_coordinator.hpp"
#include "network/types/dispute_messages.hpp"

namespace kagome::authority_discovery {
  class Query;
}

namespace kagome::dispute {
  class RuntimeInfo;
}

namespace kagome::network {
  class SendDisputeProtocol;
}

namespace kagome::dispute {

  class SendingDispute final : std::enable_shared_from_this<SendingDispute> {
   public:
    /// Initiates sending a dispute message to peers.
    ///
    /// Creation of new `SendTask`s is subject to rate limiting. As each
    /// `SendTask` will trigger sending a message to each validator, hence for
    /// employing a per-peer rate limit, we need to limit the construction of
    /// new `SendTask`s.
    SendingDispute(
        std::shared_ptr<authority_discovery::Query> authority_discovery,
        std::shared_ptr<network::SendDisputeProtocol> dispute_protocol,
        const network::DisputeMessage &request);

    /// Make sure we are sending to all relevant authorities.
    ///
    /// This function is called at construction and should also be called
    /// whenever a session change happens and on a regular basis to ensure we
    /// are retrying failed attempts.
    ///
    /// This might resend to validators and is thus subject to any rate limiting
    /// we might want. Calls to this function for different instances should be
    /// rate limited according to `SEND_RATE_LIMIT`.
    ///
    /// Returns: `True` if this call resulted in new requests.
    outcome::result<bool> refresh_sends(
        RuntimeInfo &runtime,
        std::unordered_map<SessionIndex, CandidateHash> sessions);

    bool has_failed_sends() const {
      return has_failed_sends_;
    }

   private:
    outcome::result<std::unordered_set<primitives::AuthorityDiscoveryId>>
    get_relevant_validators(
        RuntimeInfo &runtime,
        std::unordered_map<SessionIndex, CandidateHash> &sessions);

    /// Status of a particular vote/statement delivery to a particular validator
    enum DeliveryStatus {
      /// Request is still in flight.
      Pending,
      /// Succeeded - no need to send request to this peer anymore.
      Succeeded,
    };

    bool send_requests(
        std::vector<primitives::AuthorityDiscoveryId> &authorities);

    std::shared_ptr<authority_discovery::Query> authority_discovery_;
    std::weak_ptr<network::SendDisputeProtocol> dispute_protocol_;

    /// The request we are supposed to get out to all `parachain` validators of
    /// the dispute's session and to all current authorities.
    network::DisputeMessage request_;

    /// The set of authorities we need to send our messages to. This set will
    /// change at session boundaries. It will always be at least the `parachain`
    /// validators of the session where the dispute happened and the authorities
    /// of the current sessions as determined by active heads.
    std::unordered_map<primitives::AuthorityDiscoveryId, DeliveryStatus>
        deliveries_;

    /// Whether we have any tasks failed since the last refresh.
    bool has_failed_sends_ = false;
  };

}  // namespace kagome::dispute

#endif  // KAGOME_DISPUTE_SENDINGDISPUTE
