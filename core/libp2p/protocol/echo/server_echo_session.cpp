/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/protocol/echo/server_echo_session.hpp"

namespace libp2p::protocol {

  EchoSession::EchoSession(std::shared_ptr<connection::Stream> stream,
                           EchoConfig config)
      : stream_(std::move(stream)), buf_(config.max_recv_size, 0) {
    BOOST_ASSERT(stream_ != nullptr);
    BOOST_ASSERT(config.max_recv_size > 0);
  }

  void EchoSession::start() {
    doRead();
  }

  void EchoSession::stop() {
    stream_->close([self{shared_from_this()}](auto && /* ignore */) {
      // ignore result
    });
  }

  void EchoSession::doRead() {
    if (stream_->isClosedForRead()) {
      return stop();
    }

    stream_->readSome(
        buf_, buf_.size(),
        [self{shared_from_this()}](outcome::result<size_t> rread) {
          self->onRead(rread);
        });
  }

  void EchoSession::onRead(outcome::result<size_t> rread) {
    if (!rread) {
      return stop();
    }

    this->doWrite(rread.value());
  }

  void EchoSession::doWrite(size_t size) {
    if (stream_->isClosedForWrite()) {
      return stop();
    }

    stream_->write(buf_, size,
                   [self{shared_from_this()}](outcome::result<size_t> rwrite) {
                     self->onWrite(rwrite);
                   });
  }

  void EchoSession::onWrite(outcome::result<size_t> rwrite) {
    if (!rwrite) {
      return stop();
    }

    doRead();
  }
}  // namespace libp2p::protocol
