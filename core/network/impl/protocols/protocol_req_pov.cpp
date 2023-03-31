/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "network/impl/protocols/protocol_req_pov.hpp"

#include "network/common.hpp"
#include "network/impl/protocols/request_response_protocol.hpp"
#include "utils/non_copyable.hpp"

namespace kagome::network {

  struct ReqPovProtocolImpl : RequestResponseProtocol<RequestPov,
                                                      ResponsePov,
                                                      ScaleMessageReadWriter>,
                              NonCopyable,
                              NonMovable {
    ReqPovProtocolImpl(libp2p::Host &host,
                       application::ChainSpec const &chain_spec,
                       const primitives::BlockHash &genesis_hash,
                       std::shared_ptr<ReqPovObserver> observer)
        : RequestResponseProtocol<
            RequestPov,
            ResponsePov,
            ScaleMessageReadWriter>{kReqPovProtocolName,
                                    host,
                                    make_protocols(kReqPovProtocol,
                                                   genesis_hash,
                                                   "polkadot"),
                                    log::createLogger(kReqPovProtocolName,
                                                      "req_pov_protocol")},
          observer_{std::move(observer)} {}

   protected:
    outcome::result<ResponsePov> onRxRequest(
        RequestPov request, std::shared_ptr<Stream> /*stream*/) override {
      BOOST_ASSERT(observer_);
      base().logger()->info("Received PoV request(candidate hash={})", request);
      auto response = observer_->OnPovRequest(std::move(request));
      if (response.has_error()) {
        base().logger()->warn(
            "Our PoV response has error.(candidate hash={}, error={})",
            request,
            response.error().message());
      } else {
        base().logger()->info("Our PoV response {} data.(candidate hash={})",
                              boost::get<ParachainBlock>(&response.value())
                                  ? "contains"
                                  : "NOT contains",
                              request);
      }
      return response;
    }

    void onTxRequest(RequestPov const &request) override {
      base().logger()->info("Transmit PoV request(candidate hash={})", request);
    }

   private:
    const static inline auto kReqPovProtocolName = "ReqPovProtocol"s;
    std::shared_ptr<ReqPovObserver> observer_;
  };

  ReqPovProtocol::ReqPovProtocol(libp2p::Host &host,
                                 application::ChainSpec const &chain_spec,
                                 const primitives::BlockHash &genesis_hash,
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

  bool ReqPovProtocol::stop() {
    BOOST_ASSERT(impl_ && !!"ReqPovProtocolImpl must be initialized!");
    return impl_->stop();
  }

  void ReqPovProtocol::onIncomingStream(std::shared_ptr<Stream>) {
    BOOST_ASSERT(!"Must not be called!");
  }

  void ReqPovProtocol::newOutgoingStream(
      const PeerInfo &,
      std::function<void(outcome::result<std::shared_ptr<Stream>>)> &&) {
    BOOST_ASSERT(!"Must not be called!");
  }

  void ReqPovProtocol::request(
      const PeerInfo &peer_info,
      RequestPov request,
      std::function<void(outcome::result<ResponsePov>)> &&response_handler) {
    BOOST_ASSERT(impl_ && !!"ReqPovProtocolImpl must be initialized!");
    return impl_->doRequest(
        peer_info, std::move(request), std::move(response_handler));
  }

}  // namespace kagome::network
