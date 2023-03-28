/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "network/impl/protocols/protocol_req_collation.hpp"

#include "network/common.hpp"
#include "network/impl/protocols/request_response_protocol.hpp"
#include "utils/non_copyable.hpp"

namespace kagome::network {

  struct ReqCollationProtocolImpl
      : RequestResponseProtocol<CollationFetchingRequest,
                                CollationFetchingResponse,
                                ScaleMessageReadWriter>,
        NonCopyable,
        NonMovable {
    ReqCollationProtocolImpl(libp2p::Host &host,
                             application::ChainSpec const &chain_spec,
                             const primitives::BlockHash &genesis_hash,
                             std::shared_ptr<ReqCollationObserver> observer)
        : RequestResponseProtocol<
            CollationFetchingRequest,
            CollationFetchingResponse,
            ScaleMessageReadWriter>{kReqCollationProtocolName,
                                    host,
                                    make_protocols(kReqCollationProtocol,
                                                   genesis_hash,
                                                   "polkadot"),
                                    log::createLogger(
                                        kReqCollationProtocolName,
                                        "req_collation_protocol")},
          observer_{std::move(observer)} {}

   protected:
    outcome::result<CollationFetchingResponse> onRxRequest(
        CollationFetchingRequest request,
        std::shared_ptr<Stream> /*stream*/) override {
      BOOST_ASSERT(observer_);
      return observer_->OnCollationRequest(std::move(request));
    }

    void onTxRequest(CollationFetchingRequest const &request) override {
      base().logger()->debug("Requesting collation");
    }

   private:
    const static inline auto kReqCollationProtocolName =
        "ReqCollationProtocol"s;

    std::shared_ptr<ReqCollationObserver> observer_;
  };

  ReqCollationProtocol::ReqCollationProtocol(
      libp2p::Host &host,
      application::ChainSpec const &chain_spec,
      const primitives::BlockHash &genesis_hash,
      std::shared_ptr<ReqCollationObserver> observer)
      : impl_{std::make_shared<ReqCollationProtocolImpl>(
          host, chain_spec, genesis_hash, std::move(observer))} {}

  const Protocol &ReqCollationProtocol::protocolName() const {
    BOOST_ASSERT(impl_ && !!"ReqCollationProtocolImpl must be initialized!");
    return impl_->protocolName();
  }

  bool ReqCollationProtocol::start() {
    BOOST_ASSERT(impl_ && !!"ReqCollationProtocolImpl must be initialized!");
    return impl_->start();
  }

  void ReqCollationProtocol::onIncomingStream(std::shared_ptr<Stream> stream) {
    BOOST_ASSERT(!"Must not be called!");
  }

  void ReqCollationProtocol::newOutgoingStream(
      const PeerInfo &peer_info,
      std::function<void(outcome::result<std::shared_ptr<Stream>>)> &&cb) {
    BOOST_ASSERT(!"Must not be called!");
  }

  void ReqCollationProtocol::request(
      const PeerId &peer_id,
      CollationFetchingRequest request,
      std::function<void(outcome::result<CollationFetchingResponse>)>
          &&response_handler) {
    BOOST_ASSERT(impl_ && !!"ReqCollationProtocolImpl must be initialized!");
    return impl_->doRequest(
        peer_id, std::move(request), std::move(response_handler));
  }

}  // namespace kagome::network
