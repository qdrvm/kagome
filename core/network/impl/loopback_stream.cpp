/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/peer/peer_info.hpp>

#include "network/impl/loopback_stream.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::network, LoopbackStream::Error, e) {
  using E = kagome::network::LoopbackStream::Error;
  switch (e) {
    case E::INVALID_ARGUMENT:
      return "invalid argument was passed";
    case E::IS_READING:
      return "read was called before the last read completed";
    case E::IS_CLOSED_FOR_READS:
      return "this stream is closed for reads";
    case E::IS_WRITING:
      return "write was called before the last write completed";
    case E::IS_CLOSED_FOR_WRITES:
      return "this stream is closed for writes";
    case E::IS_RESET:
      return "this stream was reset";
    case E::INTERNAL_ERROR:
      return "internal error happened";
  }
  return "unknown error";
}

namespace kagome::network {
  LoopbackStream::LoopbackStream(libp2p::peer::PeerInfo own_peer_info)
      : own_peer_info_(std::move(own_peer_info)) {}

  bool LoopbackStream::isClosedForRead() const {
    return !is_readable_;
  };

  bool LoopbackStream::isClosedForWrite() const {
    return !is_writable_;
  };

  bool LoopbackStream::isClosed() const {
    return isClosedForRead() && isClosedForWrite();
  };

  void LoopbackStream::close(VoidResultHandlerFunc cb) {
    is_writable_ = false;
    cb(outcome::success());
  };

  void LoopbackStream::reset() {
    is_reset_ = true;
  };

  void LoopbackStream::adjustWindowSize(uint32_t new_size,
                                        VoidResultHandlerFunc cb){};

  outcome::result<bool> LoopbackStream::isInitiator() const {
    return outcome::success(false);
  };

  outcome::result<libp2p::peer::PeerId> LoopbackStream::remotePeerId() const {
    return outcome::success(own_peer_info_.id);
  };

  outcome::result<libp2p::multi::Multiaddress> LoopbackStream::localMultiaddr()
      const {
    return outcome::success(own_peer_info_.addresses.front());
  };

  outcome::result<libp2p::multi::Multiaddress> LoopbackStream::remoteMultiaddr()
      const {
    return outcome::success(own_peer_info_.addresses.front());
  }

  void LoopbackStream::read(gsl::span<uint8_t> out,
                            size_t bytes,
                            libp2p::basic::Reader::ReadCallbackFunc cb) {
    read(out, bytes, std::move(cb), false);
  }

  void LoopbackStream::readSome(gsl::span<uint8_t> out,
                                size_t bytes,
                                libp2p::basic::Reader::ReadCallbackFunc cb) {
    read(out, bytes, std::move(cb), true);
  }

  void LoopbackStream::write(gsl::span<const uint8_t> in,
                             size_t bytes,
                             libp2p::basic::Writer::WriteCallbackFunc cb) {
    if (is_reset_) {
      return cb(Error::IS_RESET);
    }
    if (!is_writable_) {
      return cb(Error::IS_CLOSED_FOR_WRITES);
    }
    if (bytes == 0 || in.empty() || static_cast<size_t>(in.size()) < bytes) {
      return cb(Error::INVALID_ARGUMENT);
    }
    if (is_writing_) {
      return cb(Error::IS_WRITING);
    }

    is_writing_ = true;

    auto x = read_buffer_.prepare(bytes);
    std::copy(in.data(),
              in.data() + in.size(),
              reinterpret_cast<char *>(x.data()));  // NOLINT
    read_buffer_.commit(in.size());

    is_writing_ = false;

    cb(outcome::success(in.size()));

    if (data_notifyee_) {
      data_notifyee_(read_buffer_.size());
    }
  }

  void LoopbackStream::writeSome(gsl::span<const uint8_t> in,
                                 size_t bytes,
                                 libp2p::basic::Writer::WriteCallbackFunc cb) {
    write(in, bytes, std::move(cb));
  }

  void LoopbackStream::read(gsl::span<uint8_t> out,
                            size_t bytes,
                            libp2p::basic::Reader::ReadCallbackFunc cb,
                            bool some) {
    if (is_reset_) {
      return cb(Error::IS_RESET);
    }
    if (!is_readable_) {
      return cb(Error::IS_CLOSED_FOR_READS);
    }
    if (bytes == 0 || out.empty() || static_cast<size_t>(out.size()) < bytes) {
      return cb(Error::INVALID_ARGUMENT);
    }
    if (is_reading_) {
      return cb(Error::IS_READING);
    }

    // this lambda checks, if there's enough data in our read buffer, and gives
    // it to the caller, if so
    auto read_lambda = [self{shared_from_this()},
                        cb{std::move(cb)},
                        out,
                        bytes,
                        some](outcome::result<size_t> res) mutable {
      if (!res) {
        self->data_notified_ = true;
        cb(res);
        return;
      }
      if (self->read_buffer_.size() >= (some ? 1 : bytes)) {
        auto to_read =
            some ? std::min(self->read_buffer_.size(), bytes) : bytes;
        if (boost::asio::buffer_copy(boost::asio::buffer(out.data(), to_read),
                                     self->read_buffer_.data(),
                                     to_read)
            != to_read) {
          return cb(Error::INTERNAL_ERROR);
        }

        self->is_reading_ = false;
        self->read_buffer_.consume(to_read);
        self->data_notified_ = true;
        cb(to_read);
      }
    };

    // return immediately, if there's enough data in the buffer
    data_notified_ = false;
    read_lambda(0);
    if (data_notified_) {
      return;
    }

    // subscribe to new data updates
    is_reading_ = true;
    data_notifyee_ = std::move(read_lambda);
  };

}  // namespace kagome::network
