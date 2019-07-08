/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/protocol/echo/client_echo_session.hpp"

namespace libp2p::protocol {

  ClientEchoSession::ClientEchoSession(
      std::shared_ptr<connection::Stream> stream)
      : stream_(std::move(stream)) {
    BOOST_ASSERT(stream_ != nullptr);
  }

  void ClientEchoSession::sendAnd(const std::string &send,
                                  ClientEchoSession::Then then) {
    if (stream_->isClosedForWrite()) {
      return;
    }

    buf_ = std::vector<uint8_t>(send.begin(), send.end());

    BOOST_ASSERT(stream_ != nullptr);
    stream_->write(
        buf_, buf_.size(),
        [this, then{std::move(then)}](outcome::result<size_t> rw) mutable {
          if (!rw) {
            return then(rw.error());
          }

          if (stream_->isClosedForRead()) {
            return;
          }

          stream_->read(buf_, buf_.size(),
                        [this, then{std::move(then)}](
                            outcome::result<size_t> rr) mutable {
                          if (!rr) {
                            return then(rr.error());
                          }

                          auto begin = buf_.begin();
                          auto end = buf_.begin();
                          std::advance(end, rr.value());
                          return then(std::string(begin, end));
                        });
        });
  }
}  // namespace libp2p::protocol
