/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/connection/plaintext/plaintext.hpp"

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

  void PlaintextConnection::read(gsl::span<uint8_t> in, Reader::ReadCallbackFunc f)  {
    return raw_connection_->read(in, std::move(f));
  };

  void PlaintextConnection::readSome(gsl::span<uint8_t> in, Reader::ReadCallbackFunc f)  {
    return raw_connection_->readSome(in, std::move(f));
  };

  void PlaintextConnection::write(gsl::span<const uint8_t> in,
             Writer::WriteCallbackFunc f)  {
    return raw_connection_->write(in, std::move(f));
  }

  void PlaintextConnection::writeSome(gsl::span<const uint8_t> in,
                 Writer::WriteCallbackFunc f) {
    return raw_connection_->writeSome(in, std::move(f));
  }

  bool PlaintextConnection::isClosed() const {
    return raw_connection_->isClosed();
  }

  outcome::result<void> PlaintextConnection::close() {
    return raw_connection_->close();
  }
}  // namespace libp2p::connection
