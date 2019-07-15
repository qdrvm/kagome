/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/protocol/ping/ping_client_session.hpp"

#include <boost/assert.hpp>
#include "libp2p/protocol/ping/common.hpp"

namespace libp2p::protocol {
  using detail::kPingMsgSize;

  PingClientSession::PingClientSession(
      std::shared_ptr<connection::Stream> stream,
      std::shared_ptr<crypto::random::RandomGenerator> rand_gen)
      : stream_{std::move(stream)},
        rand_gen_{std::move(rand_gen)},
        write_buffer_(kPingMsgSize, 0),
        read_buffer_(kPingMsgSize, 0) {
    BOOST_ASSERT(stream_);
    BOOST_ASSERT(rand_gen_);
  }

  void PingClientSession::start() {
    BOOST_ASSERT(!is_started_);
    is_started_ = true;
    write();
  }

  void PingClientSession::stop() {
    BOOST_ASSERT(is_started_);
    is_started_ = false;
  }

  void PingClientSession::write() {
    if (!is_started_ || stream_->isClosedForWrite()) {
      return;
    }

    auto rand_buf = rand_gen_->randomBytes(kPingMsgSize);
    std::move(rand_buf.begin(), rand_buf.end(), write_buffer_.begin());
    stream_->write(write_buffer_, kPingMsgSize,
                   [self{shared_from_this()}](auto &&write_res) {
                     if (!write_res) {
                       return;
                     }
                     self->writeCompleted();
                   });
  }

  void PingClientSession::writeCompleted() {
    read();
  }

  void PingClientSession::read() {
    if (!is_started_ || stream_->isClosedForRead()) {
      return;
    }

    stream_->read(read_buffer_, kPingMsgSize,
                  [self{shared_from_this()}](auto &&read_res) {
                    if (!read_res) {
                      return;
                    }
                    self->readCompleted();
                  });
  }

  void PingClientSession::readCompleted() {
    if (write_buffer_ != read_buffer_) {
      return;
    }
    write();
  }
}  // namespace libp2p::protocol
