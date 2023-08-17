/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_NETWORK_SENDDISPUTEPROTOCOLIMPL
#define KAGOME_NETWORK_SENDDISPUTEPROTOCOLIMPL

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
#include "network/impl/stream_engine.hpp"
#include "network/types/dispute_messages.hpp"
#include "utils/non_copyable.hpp"

namespace kagome::network {

  struct ReqPovProtocolImpl;

  class SendDisputeProtocol final
      : public RequestResponseProtocol<DisputeMessage,
                                       boost::variant<kagome::Empty>,
                                       ScaleMessageReadWriter>,
        NonCopyable,
        NonMovable {
   public:
    SendDisputeProtocol() = delete;
    ~SendDisputeProtocol() override = default;

    SendDisputeProtocol(libp2p::Host &host,
                        const blockchain::GenesisBlockHash &genesis_hash,
                        std::shared_ptr<network::DisputeRequestObserver>
                            dispute_request_observer)
        : RequestResponseProtocol<
            DisputeMessage,
            boost::variant<kagome::Empty>,
            ScaleMessageReadWriter>{kSendDisputeProtocolName,
                                    host,
                                    make_protocols(kSendDisputeProtocol,
                                                   genesis_hash,
                                                   "polkadot"),
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

      BOOST_ASSERT(stream->remotePeerId().has_value());

      dispute_request_observer_->onDisputeRequest(
          stream->remotePeerId().value(),
          std::move(request),
          [wp = weak_from_this(),
           &logger = base().logger(),
           write = &SendDisputeProtocol::writeResponse,
           stream{std::move(stream)}](outcome::result<void> res) {
            if (auto self = wp.lock()) {
              if (res.has_error()) {
                SL_WARN(logger,
                        "Processing dispute request failed: {}",
                        res.error());
                stream->reset();
                return;
              }

              SL_TRACE(logger, "Processing dispute request successful.");
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

#endif  // KAGOME_NETWORK_SENDDISPUTEPROTOCOLIMPL
