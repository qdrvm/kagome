/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_PING_HPP
#define KAGOME_PING_HPP

#include <string_view>

#include "libp2p/host.hpp"
#include "libp2p/protocol/base_protocol.hpp"

namespace libp2p::protocol {
  /**
   * Ping protocol, which is used to understand, if the peers are alive
   */
  class Ping : public BaseProtocol {
   public:
    explicit Ping(Host &host);

    peer::Protocol getProtocolId() const override;

    void handle(StreamResult res) override;

    void startPinging(std::shared_ptr<connection::Stream> stream);

   private:
    static constexpr std::string_view kPingProto = "/ipfs/ping/1.0.0";

    Host &host_;
  };
}  // namespace libp2p::protocol

#endif  // KAGOME_PING_HPP
