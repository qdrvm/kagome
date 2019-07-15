/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_PING_CLIENT_SESSION_HPP
#define KAGOME_PING_CLIENT_SESSION_HPP

#include <memory>
#include <vector>

#include "libp2p/connection/stream.hpp"
#include "libp2p/crypto/random_generator.hpp"

namespace libp2p::protocol {
  class PingClientSession
      : public std::enable_shared_from_this<PingClientSession> {
   public:
    PingClientSession(
        std::shared_ptr<connection::Stream> stream,
        std::shared_ptr<crypto::random::RandomGenerator> rand_gen);

    void start();

    void stop();

   private:
    void write();

    void writeCompleted();

    void read();

    void readCompleted();

    std::shared_ptr<connection::Stream> stream_;
    std::shared_ptr<crypto::random::RandomGenerator> rand_gen_;

    std::vector<uint8_t> write_buffer_, read_buffer_;

    bool is_started_ = false;
  };
}  // namespace libp2p::protocol

#endif  // KAGOME_PING_CLIENT_SESSION_HPP
