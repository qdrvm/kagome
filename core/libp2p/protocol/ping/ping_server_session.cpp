/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/protocol/ping/ping_server_session.hpp"

#include <boost/assert.hpp>

namespace libp2p::protocol {
  PingServerSession::PingServerSession(
      Host &host, std::shared_ptr<connection::Stream> stream)
      : host_{host}, stream_{std::move(stream)}, buffer_(kPingMsgSize, 0) {
    BOOST_ASSERT(stream_);
  }

  void PingServerSession::start() {
    BOOST_ASSERT(!is_started_);
    is_started_ = true;

    read();
  }

  void PingServerSession::read() {
    stream_->read(buffer_, kPingMsgSize,
                  [self{shared_from_this()}](auto &&read_res) {
                    if (!read_res) {
                      return;
                    }
                    self->readCompleted();
                  });
  }

  void PingServerSession::readCompleted() {
    write();
  }

  void PingServerSession::write() {
    stream_->write(buffer_, kPingMsgSize,
                   [self{shared_from_this()}](auto &&write_res) {
                     if (!write_res) {
                       return;
                     }
                     self->writeCompleted();
                   });
  }

  void PingServerSession::writeCompleted() {
    read();
  }
}  // namespace libp2p::protocol
