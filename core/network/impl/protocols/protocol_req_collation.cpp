/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "network/impl/protocols/protocol_req_collation.hpp"

#include "blockchain/genesis_block_hash.hpp"
#include "network/common.hpp"
#include "network/impl/protocols/request_response_protocol.hpp"
#include "utils/non_copyable.hpp"

namespace kagome::network {

  template <typename RequestT, typename ResponseT>
  struct ReqCollationProtocolImpl
      : RequestResponseProtocol<std::decay_t<RequestT>,
                                std::decay_t<ResponseT>,
                                ScaleMessageReadWriter>,
        NonCopyable,
        NonMovable {
    using Base = RequestResponseProtocol<std::decay_t<RequestT>,
                                         std::decay_t<ResponseT>,
                                         ScaleMessageReadWriter>;

    ReqCollationProtocolImpl(libp2p::Host &host,
                             const application::ChainSpec &chain_spec,
                             const blockchain::GenesisBlockHash &genesis_hash,
                             std::shared_ptr<ReqCollationObserver> observer)
        : Base{kReqCollationProtocolName,
               host,
               make_protocols(kReqCollationProtocol, genesis_hash, "polkadot"),
               log::createLogger(kReqCollationProtocolName,
                                 "req_collation_protocol")},
          observer_{std::move(observer)} {}

   protected:
    std::optional<outcome::result<typename Base::ResponseType>> onRxRequest(
        typename Base::RequestType request,
        std::shared_ptr<Stream> /*stream*/) override {
      BOOST_ASSERT(observer_);
      return observer_->OnCollationRequest(std::move(request));
    }

    void onTxRequest(const typename Base::RequestType &request) override {
      Base::base().logger()->debug("Requesting collation");
    }

   private:
    inline static const auto kReqCollationProtocolName =
        "ReqCollationProtocol"s;

    std::shared_ptr<ReqCollationObserver> observer_;
  };

  ReqCollationProtocol::ReqCollationProtocol(
      libp2p::Host &host,
      const application::ChainSpec &chain_spec,
      const blockchain::GenesisBlockHash &genesis_hash,
      std::shared_ptr<ReqCollationObserver> observer)
      : v1_impl_{std::make_shared<
          ReqCollationProtocolImpl<CollationFetchingRequest,
                                   CollationFetchingResponse>>(
          host, chain_spec, genesis_hash, observer)},
        vstaging_impl_{std::make_shared<
            ReqCollationProtocolImpl<vstaging::CollationFetchingRequest,
                                     vstaging::CollationFetchingResponse>>(
            host, chain_spec, genesis_hash, observer)} {
    BOOST_ASSERT(v1_impl_);
    BOOST_ASSERT(vstaging_impl_);
  }

  const Protocol &ReqCollationProtocol::protocolName() const {
    BOOST_ASSERT_MSG(v1_impl_, "ReqCollationProtocolImpl must be initialized!");
    return v1_impl_->protocolName();
  }

  bool ReqCollationProtocol::start() {
    BOOST_ASSERT_MSG(v1_impl_,
                     "v1 ReqCollationProtocolImpl must be initialized!");
    BOOST_ASSERT_MSG(vstaging_impl_,
                     "vstaging ReqCollationProtocolImpl must be initialized!");
    return v1_impl_->start() && vstaging_impl_->start();
  }

  void ReqCollationProtocol::onIncomingStream(std::shared_ptr<Stream> stream) {}

  void ReqCollationProtocol::newOutgoingStream(
      const PeerInfo &peer_info,
      std::function<void(outcome::result<std::shared_ptr<Stream>>)> &&cb) {
    BOOST_ASSERT_MSG(false, "Must not be called!");
  }

  void ReqCollationProtocol::request(
      const PeerId &peer_id,
      CollationFetchingRequest request,
      std::function<void(outcome::result<CollationFetchingResponse>)>
          &&response_handler) {
    BOOST_ASSERT_MSG(v1_impl_,
                     "v1 ReqCollationProtocolImpl must be initialized!");
    return v1_impl_->doRequest(
        peer_id, std::move(request), std::move(response_handler));
  }

  void ReqCollationProtocol::request(
      const PeerId &peer_id,
      vstaging::CollationFetchingRequest request,
      std::function<void(outcome::result<vstaging::CollationFetchingResponse>)>
          &&response_handler) {
    BOOST_ASSERT_MSG(vstaging_impl_,
                     "vstaging ReqCollationProtocolImpl must be initialized!");
    return vstaging_impl_->doRequest(
        peer_id, std::move(request), std::move(response_handler));
  }

}  // namespace kagome::network
