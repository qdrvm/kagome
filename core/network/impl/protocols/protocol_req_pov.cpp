/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "network/impl/protocols/protocol_req_pov.hpp"

#include "blockchain/genesis_block_hash.hpp"
#include "network/common.hpp"
#include "network/helpers/scale_message_read_writer.hpp"
#include "network/impl/protocols/request_response_protocol.hpp"
#include "utils/non_copyable.hpp"

namespace kagome::network {

  struct ReqPovProtocolImpl
      : RequestResponseProtocolImpl<RequestPov,
                                    ResponsePov,
                                    ScaleMessageReadWriter>,
        NonCopyable,
        NonMovable {
    ReqPovProtocolImpl(libp2p::Host &host,
                       const application::ChainSpec &chain_spec,
                       const blockchain::GenesisBlockHash &genesis_hash,
                       std::shared_ptr<ReqPovObserver> observer)
        : RequestResponseProtocolImpl<
            RequestPov,
            ResponsePov,
            ScaleMessageReadWriter>{kReqPovProtocolName,
                                    host,
                                    make_protocols(kReqPovProtocol,
                                                   genesis_hash,
                                                   kProtocolPrefixPolkadot),
                                    log::createLogger(kReqPovProtocolName,
                                                      "req_pov_protocol")},
          observer_{std::move(observer)} {}

   protected:
    std::optional<outcome::result<ResponsePov>> onRxRequest(
        RequestPov request, std::shared_ptr<Stream> /*stream*/) override {
      BOOST_ASSERT(observer_);
      base().logger()->info("Received PoV request(candidate hash={})", request);
      auto response = observer_->OnPovRequest(request);
      if (response.has_error()) {
        base().logger()->warn(
            "Our PoV response has error.(candidate hash={}, error={})",
            request,
            response.error());
      } else {
        base().logger()->info("Our PoV response {} data.(candidate hash={})",
                              boost::get<ParachainBlock>(&response.value())
                                  ? "contains"
                                  : "NOT contains",
                              request);
      }
      return response;
    }

    void onTxRequest(const RequestPov &request) override {
      SL_TRACE(
          base().logger(), "Transmit PoV request(candidate hash={})", request);
    }

   private:
    inline static const auto kReqPovProtocolName = "ReqPovProtocol"s;
    std::shared_ptr<ReqPovObserver> observer_;
  };

  ReqPovProtocol::ReqPovProtocol(
      libp2p::Host &host,
      const application::ChainSpec &chain_spec,
      const blockchain::GenesisBlockHash &genesis_hash,
      std::shared_ptr<ReqPovObserver> observer)
      : impl_{std::make_shared<ReqPovProtocolImpl>(
          host, chain_spec, genesis_hash, std::move(observer))} {}

  const Protocol &ReqPovProtocol::protocolName() const {
    BOOST_ASSERT(impl_ && !!"ReqPovProtocolImpl must be initialized!");
    return impl_->protocolName();
  }

  bool ReqPovProtocol::start() {
    BOOST_ASSERT(impl_ && !!"ReqPovProtocolImpl must be initialized!");
    return impl_->start();
  }

  void ReqPovProtocol::onIncomingStream(std::shared_ptr<Stream>) {
    BOOST_ASSERT(!"Must not be called!");
  }

  void ReqPovProtocol::newOutgoingStream(
      const PeerId &,
      std::function<void(outcome::result<std::shared_ptr<Stream>>)> &&) {
    BOOST_ASSERT(!"Must not be called!");
  }

  void ReqPovProtocol::request(
      const PeerId &peer_id,
      RequestPov request,
      std::function<void(outcome::result<ResponsePov>)> &&response_handler) {
    BOOST_ASSERT(impl_ && !!"ReqPovProtocolImpl must be initialized!");
    return impl_->doRequest(peer_id, request, std::move(response_handler));
  }

}  // namespace kagome::network
