/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/protocol_muxer/multiselect/connection_state.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(libp2p::protocol_muxer,
                            ConnectionState::ReadWriteError, e) {
  using E = libp2p::protocol_muxer::ConnectionState::ReadWriteError;
  switch (e) {
    case E::PARTIAL_WRITE:
      return "less bytes were written than expected";
  }
  return "unknown error";
}

namespace libp2p::protocol_muxer {
  ConnectionState::ConnectionState(
      std::shared_ptr<basic::ReadWriteCloser> conn,
      ProtocolMuxer::ChosenProtocolCallback proto_cb,
      std::shared_ptr<kagome::common::Buffer> write_buffer,
      size_t buffers_index, std::shared_ptr<Multiselect> multiselect,
      NegotiationRound round, NegotiationStatus status)
      : connection_{std::move(conn)},
        proto_callback_{std::move(proto_cb)},
        write_buffer_{std::move(write_buffer)},
        buffers_index_{buffers_index},
        multiselect_{std::move(multiselect)},
        round_{round},
        status_{status} {}

  outcome::result<void> ConnectionState::write() {
    OUTCOME_TRY(written, connection_->write(write_buffer_->toVector()));
    if (written != write_buffer_->size()) {
      return ReadWriteError::PARTIAL_WRITE;
    }
    return outcome::success();
  }

  outcome::result<std::vector<uint8_t>> ConnectionState::read(size_t n) {
    return connection_->read(n);
  }

}  // namespace libp2p::protocol_muxer
