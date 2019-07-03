/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/connection/plaintext/plaintext.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(libp2p::connection, PlaintextConnection::Error, e) {
  using E = libp2p::connection::PlaintextConnection::Error;
  switch (e) {
    case E::FIELD_IS_UNSUPPORTED:
      return "this field is either not supported or not set in this connection";
  }
  return "unknown error";
}

namespace libp2p::connection {
  PlaintextConnection::PlaintextConnection(
      std::shared_ptr<RawConnection> raw_connection)
      : raw_connection_{std::move(raw_connection)} {}

  PlaintextConnection::PlaintextConnection(
      std::shared_ptr<RawConnection> raw_connection, peer::PeerId peer_id)
      : raw_connection_{std::move(raw_connection)},
        peer_id_{std::move(peer_id)} {}

  outcome::result<peer::PeerId> PlaintextConnection::localPeer() const {
    return Error::FIELD_IS_UNSUPPORTED;
  }

  outcome::result<peer::PeerId> PlaintextConnection::remotePeer() const {
    if (peer_id_) {
      return *peer_id_;
    }
    return Error::FIELD_IS_UNSUPPORTED;
  }

  outcome::result<crypto::PublicKey> PlaintextConnection::remotePublicKey()
      const {
    return Error::FIELD_IS_UNSUPPORTED;
  }

  bool PlaintextConnection::isInitiator() const noexcept {
    return raw_connection_->isInitiator();
  }

  outcome::result<multi::Multiaddress> PlaintextConnection::localMultiaddr() {
    return raw_connection_->localMultiaddr();
  }

  outcome::result<multi::Multiaddress> PlaintextConnection::remoteMultiaddr() {
    return raw_connection_->remoteMultiaddr();
  }

  void PlaintextConnection::read(gsl::span<uint8_t> in, size_t bytes,
                                 Reader::ReadCallbackFunc f) {
    return raw_connection_->read(in, bytes, std::move(f));
  };

  void PlaintextConnection::readSome(gsl::span<uint8_t> in, size_t bytes,
                                     Reader::ReadCallbackFunc f) {
    return raw_connection_->readSome(in, bytes, std::move(f));
  };

  void PlaintextConnection::write(gsl::span<const uint8_t> in, size_t bytes,
                                  Writer::WriteCallbackFunc f) {
    return raw_connection_->write(in, bytes, std::move(f));
  }

  void PlaintextConnection::writeSome(gsl::span<const uint8_t> in, size_t bytes,
                                      Writer::WriteCallbackFunc f) {
    return raw_connection_->writeSome(in, bytes, std::move(f));
  }

  bool PlaintextConnection::isClosed() const {
    return raw_connection_->isClosed();
  }

  outcome::result<void> PlaintextConnection::close() {
    return raw_connection_->close();
  }
}  // namespace libp2p::connection
