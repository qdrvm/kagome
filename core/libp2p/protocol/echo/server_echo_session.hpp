/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_SERVER_ECHO_SESSION_HPP
#define KAGOME_SERVER_ECHO_SESSION_HPP

#include <vector>

#include <boost/assert.hpp>
#include "libp2p/connection/stream.hpp"
#include "libp2p/protocol/echo/echo_config.hpp"

namespace libp2p::protocol {

  class EchoSession : public std::enable_shared_from_this<EchoSession> {
   public:
    explicit EchoSession(std::shared_ptr<connection::Stream> stream,
                         EchoConfig config = {});
    void start();

    void stop();

   private:
    std::shared_ptr<connection::Stream> stream_;
    std::vector<uint8_t> buf_;

    void doRead();

    void onRead(outcome::result<size_t> rread);

    void doWrite(size_t size);

    void onWrite(outcome::result<size_t> rwrite);
  };

}  // namespace libp2p::protocol

#endif  //KAGOME_SERVER_ECHO_SESSION_HPP
