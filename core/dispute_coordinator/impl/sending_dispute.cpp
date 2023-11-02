/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "dispute_coordinator/impl/sending_dispute.hpp"
#include "authority_discovery/query/query.hpp"
#include "dispute_coordinator/impl/runtime_info.hpp"
#include "network/impl/protocols/send_dispute_protocol.hpp"

namespace kagome::dispute {

  SendingDispute::SendingDispute(
      std::shared_ptr<authority_discovery::Query> authority_discovery,
      std::shared_ptr<network::SendDisputeProtocol> dispute_protocol,
      const network::DisputeMessage &request)
      : authority_discovery_(std::move(authority_discovery)),
        dispute_protocol_(std::move(dispute_protocol)),
        request_(std::move(request)) {
    BOOST_ASSERT(authority_discovery_ != nullptr);
    BOOST_ASSERT(dispute_protocol_.lock());
  }

  outcome::result<bool> SendingDispute::refresh_sends(
      RuntimeInfo &runtime,
      std::unordered_map<SessionIndex, CandidateHash> active_sessions) {
    OUTCOME_TRY(new_authorities,
                get_relevant_validators(runtime, active_sessions));

    // Note this will also contain all authorities for which sending failed
    // previously:
    std::vector<primitives::AuthorityDiscoveryId> add_authorities;
    std::for_each(new_authorities.begin(),
                  new_authorities.end(),
                  [&](const auto &authority) {
                    if (deliveries_.count(authority) == 0) {
                      add_authorities.emplace_back(authority);
                    }
                  });

    // Get rid of dead/irrelevant tasks/statuses:
    // SL_TRACE "Cleaning up deliveries"
    for (auto it = deliveries_.begin(); it != deliveries_.end();) {
      if (new_authorities.count(it->first) == 0) {
        it = deliveries_.erase(it);
      } else {
        ++it;
      }
    }

    // Start any new tasks that are needed:

    // SL_TRACE(log_,
    //          "Starting new send requests for authorities. "
    //          "(new_and_failed_authorities={},overall_authority_set_size={},"
    //          "already_running_deliveries={})",
    //          add_authorities.size(),
    //          new_authorities.size(),
    //          deliveries_.size());

    auto sent = send_requests(add_authorities);

    return sent;
  }

  outcome::result<std::unordered_set<primitives::AuthorityDiscoveryId>>
  SendingDispute::get_relevant_validators(
      RuntimeInfo &runtime,
      std::unordered_map<SessionIndex, CandidateHash> &active_sessions) {
    std::unordered_set<primitives::AuthorityDiscoveryId> authorities;

    auto retrieve = [&](auto &head,
                        auto session_index) -> outcome::result<void> {
      OUTCOME_TRY(ext_session_info,
                  runtime.get_session_info_by_index(head, session_index));

      auto &session_info = ext_session_info.session_info;

      auto our_index = ext_session_info.validator_info.our_index;

      std::for_each(session_info.discovery_keys.begin(),
                    session_info.discovery_keys.end(),
                    [&, index = 0](const auto &key) mutable {
                      if (index++ != our_index) {
                        authorities.emplace(key);
                      }
                    });
      return outcome::success();
    };

    // Retrieve all authorities which participated in the parachain consensus of
    // the session in which the candidate was backed.
    OUTCOME_TRY(retrieve(request_.candidate_receipt.descriptor.relay_parent,
                         request_.session_index));

    // Retrieve all authorities for the current session as indicated by the
    // active heads we are tracking.
    for (auto &[session_index, head] : active_sessions) {
      OUTCOME_TRY(retrieve(head, session_index));
    }

    return authorities;
  }

  bool SendingDispute::send_requests(
      std::vector<primitives::AuthorityDiscoveryId> &authorities) {
    std::vector<
        std::tuple<primitives::AuthorityDiscoveryId, libp2p::peer::PeerId>>
        receivers;
    for (auto &authority_id : std::move(authorities)) {
      if (auto peer_info = authority_discovery_->get(authority_id)) {
        receivers.emplace_back(authority_id, peer_info->id);
      }
    }

    if (receivers.empty()) {
      // SL_WARN(log_, "No known peers to receive dispute request");
      return false;
    }

    auto protocol = dispute_protocol_.lock();
    BOOST_ASSERT_MSG(protocol, "Protocol `send dispute` has gone");
    if (not protocol) {
      return false;
    }

    has_failed_sends_ = false;

    for (auto &[authority_id, peer_id] : receivers) {
      deliveries_.emplace(authority_id, DeliveryStatus::Pending);
      protocol->doRequest(peer_id,
                          request_,
                          [wp = weak_from_this(),
                           authority_id(authority_id)](auto res) mutable {
                            if (auto self = wp.lock()) {
                              if (res.has_value()) {
                                self->deliveries_[authority_id] =
                                    DeliveryStatus::Succeeded;
                              } else {
                                // LOG Can't sent dispute request to peer {}
                                self->deliveries_.erase(authority_id);
                                self->has_failed_sends_ = true;
                              }
                            }
                          });
    }

    // SL_TRACE(log_, "Requests dispatched. Sent {} requests",
    // receivers.size());

    return true;
  }

}  // namespace kagome::dispute
