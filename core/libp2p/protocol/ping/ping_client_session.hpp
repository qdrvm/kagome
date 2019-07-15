/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_PING_CLIENT_SESSION_HPP
#define KAGOME_PING_CLIENT_SESSION_HPP

#include <memory>

#include "libp2p/connection/stream.hpp"

namespace libp2p::protocol {
  class PingClientSession
      : public std::enable_shared_from_this<PingClientSession> {
   public:
    explicit PingClientSession(std::shared_ptr<connection::Stream> stream);

    void start();

    void stop();

   private:
    std::shared_ptr<connection::Stream> stream_;
  };
}  // namespace libp2p::protocol

#endif  // KAGOME_PING_CLIENT_SESSION_HPP
