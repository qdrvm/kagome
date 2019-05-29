/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/connection/security_conn_impl/plaintext_connection.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(libp2p::connection, PlaintextConnection::Error, e) {
  using E = libp2p::connection::PlaintextConnection::Error;
  switch (e) {
    case E::FIELD_IS_UNSUPPORTED:
      return "this field is not supported in this connection implementation";
  }
  return "unknown error";
}

namespace libp2p::connection {
  PlaintextConnection::PlaintextConnection(
      std::shared_ptr<RawConnection> raw_connection)
      : raw_connection_{std::move(raw_connection)} {}

  outcome::result<peer::PeerId> PlaintextConnection::localPeer() const {
    return Error::FIELD_IS_UNSUPPORTED;
  }

  outcome::result<peer::PeerId> PlaintextConnection::remotePeer() const {
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

  outcome::result<std::vector<uint8_t>> PlaintextConnection::read(
      size_t bytes) {
    return raw_connection_->read(bytes);
  }

  outcome::result<std::vector<uint8_t>> PlaintextConnection::readSome(
      size_t bytes) {
    return raw_connection_->readSome(bytes);
  }

  outcome::result<size_t> PlaintextConnection::read(gsl::span<uint8_t> buf) {
    return raw_connection_->read(buf);
  }

  outcome::result<size_t> PlaintextConnection::readSome(
      gsl::span<uint8_t> buf) {
    return raw_connection_->readSome(buf);
  }

  outcome::result<size_t> PlaintextConnection::write(
      gsl::span<const uint8_t> in) {
    return raw_connection_->write(in);
  }

  outcome::result<size_t> PlaintextConnection::writeSome(
      gsl::span<const uint8_t> in) {
    return raw_connection_->writeSome(in);
  }

  bool PlaintextConnection::isClosed() const {
    return raw_connection_->isClosed();
  }

  outcome::result<void> PlaintextConnection::close() {
    return raw_connection_->close();
  }
}  // namespace libp2p::connection
