/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "network/impl/protocols/request_response_protocol.hpp"

#include "network/protocol_base.hpp"

#include <memory>

#include <libp2p/connection/stream.hpp>
#include <libp2p/host/host.hpp>

#include "blockchain/genesis_block_hash.hpp"
#include "common/empty.hpp"
#include "dispute_coordinator/dispute_coordinator.hpp"
#include "log/logger.hpp"
#include "network/common.hpp"
#include "network/dispute_request_observer.hpp"
#include "network/helpers/scale_message_read_writer.hpp"
#include "network/types/dispute_messages.hpp"
#include "utils/non_copyable.hpp"

namespace kagome::network {

  using DisputeRequest = DisputeMessage;
  using DisputeResponse = boost::variant<kagome::Empty>;

  class SendDisputeProtocol
      : virtual public RequestResponseProtocol<DisputeRequest,
                                               DisputeResponse> {};

  class SendDisputeProtocolImpl final
      : public SendDisputeProtocol,
        public RequestResponseProtocolImpl<DisputeRequest,
                                           DisputeResponse,
                                           ScaleMessageReadWriter>,
        NonCopyable,
        NonMovable {
   public:
    SendDisputeProtocolImpl(libp2p::Host &host,
                            const blockchain::GenesisBlockHash &genesis_hash,
                            std::shared_ptr<network::DisputeRequestObserver>
                                dispute_request_observer)
        : RequestResponseProtocolImpl<
            DisputeRequest,
            DisputeResponse,
            ScaleMessageReadWriter>{kSendDisputeProtocolName,
                                    host,
                                    make_protocols(kSendDisputeProtocol,
                                                   genesis_hash,
                                                   kProtocolPrefixPolkadot),
                                    log::createLogger(kSendDisputeProtocolName,
                                                      "dispute_protocol")},
          dispute_request_observer_{std::move(dispute_request_observer)} {
      BOOST_ASSERT(dispute_request_observer_);
    }

   private:
    std::optional<outcome::result<ResponseType>> onRxRequest(
        RequestType request, std::shared_ptr<Stream> stream) override {
      SL_INFO(base().logger(),
              "Processing dispute request.(candidate={}, session={})",
              request.candidate_receipt.commitments_hash,
              request.session_index);

      BOOST_ASSERT(stream);
      BOOST_ASSERT(stream->remotePeerId().has_value());
      auto peer_id = stream->remotePeerId().value();

      dispute_request_observer_->onDisputeRequest(
          peer_id,
          std::move(request),
          [wp{weak_from_this()},
           base = &SendDisputeProtocolImpl::base,
           write = &SendDisputeProtocolImpl::writeResponse,
           stream = std::move(stream)](outcome::result<void> res) mutable {
            BOOST_ASSERT(stream);

            if (auto self = wp.lock()) {
              if (res.has_error()) {
                SL_WARN(((*self).*base)().logger(),
                        "Processing dispute request failed: {}",
                        res.error());
                ((*self).*base)().closeStream(wp, std::move(stream));
                return;
              }

              SL_TRACE(((*self).*base)().logger(),
                       "Processing dispute request successful");
              ((*self).*write)(std::move(stream),
                               ResponseType{kagome::Empty{}});
            }
          });

      return std::nullopt;
    }

    void onTxRequest(const RequestType &request) override {
      SL_DEBUG(base().logger(),
               "Processing dispute request.(candidate={}, session={})",
               request.candidate_receipt.commitments_hash,
               request.session_index);
    }

    inline static const auto kSendDisputeProtocolName = "DisputeProtocol"s;
    std::shared_ptr<network::DisputeRequestObserver> dispute_request_observer_;
  };

}  // namespace kagome::network
