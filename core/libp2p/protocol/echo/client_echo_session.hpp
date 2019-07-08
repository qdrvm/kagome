/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CLIENT_ECHO_SESSION_HPP
#define KAGOME_CLIENT_ECHO_SESSION_HPP

#include <vector>

#include <boost/assert.hpp>
#include "libp2p/connection/stream.hpp"
#include "libp2p/protocol/echo/echo_config.hpp"

namespace libp2p::protocol {

  class ClientEchoSession
      : public std::enable_shared_from_this<ClientEchoSession> {
   public:
    using Then = std::function<void(outcome::result<std::string>)>;

    explicit ClientEchoSession(std::shared_ptr<connection::Stream> stream);

    void sendAnd(const std::string &send, Then then);

   private:
    std::shared_ptr<connection::Stream> stream_;
    std::vector<uint8_t> buf_;
  };

}  // namespace libp2p::protocol

#endif  // KAGOME_CLIENT_ECHO_SESSION_HPP
