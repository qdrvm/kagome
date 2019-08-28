/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_RPC_SENDER_LIBP2P_HPP
#define KAGOME_RPC_SENDER_LIBP2P_HPP

#include "common/logger.hpp"
#include "libp2p/host/host.hpp"
#include "libp2p/peer/peer_info.hpp"
#include "libp2p/peer/protocol.hpp"
#include "network/rpc_sender.hpp"

namespace libp2p::basic {
  class MessageReadWriter;
}

namespace kagome::network {
  struct RPCInfoLibp2p {
    libp2p::peer::PeerInfo peer_info;
    libp2p::peer::Protocol protocol;
  };

  class RPCSenderLibp2p : public RPCSender<RPCInfoLibp2p>,
                          public std::enable_shared_from_this<RPCSenderLibp2p> {
   public:
    explicit RPCSenderLibp2p(
        libp2p::Host &host,
        common::Logger logger = common::createLogger("RPCSenderLibp2p"));

    ~RPCSenderLibp2p() override = default;

    void sendWithResponse(RPCInfoLibp2p rpc_info,
                          BufferSPtr request,
                          Callback cb) const override;

    void sendWithoutResponse(
        RPCInfoLibp2p rpc_info,
        BufferSPtr request,
        std::function<void(outcome::result<void>)> cb) const override;

   private:
    void receive(
        outcome::result<size_t> send_res,
        const std::shared_ptr<libp2p::basic::MessageReadWriter> &read_writer,
        Callback cb) const;

    libp2p::Host &host_;
    common::Logger log_;
  };
}  // namespace kagome::network

#endif  // KAGOME_RPC_SENDER_LIBP2P_HPP
