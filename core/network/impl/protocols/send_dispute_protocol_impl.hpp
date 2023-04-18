/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_NETWORK_SENDDISPUTEPROTOCOLIMPL
#define KAGOME_NETWORK_SENDDISPUTEPROTOCOLIMPL

#include "network/protocols/send_dispute_protocol.hpp"
#include "utils/non_copyable.hpp"

#include <libp2p/host/host.hpp>

#include "application/chain_spec.hpp"
#include "network/dispute_request_observer.hpp"
#include "network/reputation_repository.hpp"
#include "protocol_base_impl.hpp"

namespace kagome::network {

  class SendDisputeProtocolImpl final
      : public SendDisputeProtocol,
        public std::enable_shared_from_this<SendDisputeProtocolImpl>,
        NonCopyable,
        NonMovable {
   public:
    using DisputeResponse = boost::variant<Empty>;

    SendDisputeProtocolImpl(
        libp2p::Host &host,
        const application::ChainSpec &chain_spec,
        const primitives::BlockHash &genesis_hash,
        std::shared_ptr<DisputeRequestObserver> dispute_observer,
        std::shared_ptr<ReputationRepository> reputation_repository);

    bool start() override;

    const std::string &protocolName() const override {
      return kDisputeProtocolName;
    }

    void onIncomingStream(std::shared_ptr<Stream> stream) override;
    void newOutgoingStream(
        const PeerInfo &peer_info,
        std::function<void(outcome::result<std::shared_ptr<Stream>>)> &&cb)
        override;

    void request(
        const PeerId &peer_id,
        DisputeRequest dispute_request,
        std::function<void(outcome::result<void>)> &&response_handler) override;

    void readRequest(std::shared_ptr<Stream> dispute_request_res);

    void writeResponse(std::shared_ptr<Stream> stream);

    void writeRequest(std::shared_ptr<Stream> stream,
                      DisputeRequest dispute_request,
                      std::function<void(outcome::result<void>)> &&cb);

    void readResponse(
        std::shared_ptr<Stream> stream,
        std::function<void(outcome::result<void>)> &&response_handler);

   private:
    const static inline auto kDisputeProtocolName = "DisputeProtocol"s;
    ProtocolBaseImpl base_;
    std::shared_ptr<DisputeRequestObserver> dispute_observer_;
    std::shared_ptr<ReputationRepository> reputation_repository_;
  };

}  // namespace kagome::network

#endif  // KAGOME_NETWORK_SENDDISPUTEPROTOCOLIMPL
