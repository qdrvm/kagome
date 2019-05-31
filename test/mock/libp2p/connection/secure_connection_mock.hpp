/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_SECURE_CONNECTION_MOCK_HPP
#define KAGOME_SECURE_CONNECTION_MOCK_HPP

#include <gmock/gmock.h>
#include "libp2p/connection/secure_connection.hpp"

namespace libp2p::connection {
  class SecureConnectionMock : public SecureConnection {
   public:
    MOCK_CONST_METHOD0(isClosed, bool(void));

    MOCK_METHOD0(close, outcome::result<void>(void));

    MOCK_METHOD1(write, outcome::result<size_t>(gsl::span<const uint8_t>));

    MOCK_METHOD1(writeSome, outcome::result<size_t>(gsl::span<const uint8_t>));

    MOCK_METHOD1(read, outcome::result<std::vector<uint8_t>>(size_t));

    MOCK_METHOD1(readSome, outcome::result<std::vector<uint8_t>>(size_t));

    MOCK_METHOD1(read, outcome::result<size_t>(gsl::span<uint8_t>));

    MOCK_METHOD1(readSome, outcome::result<size_t>(gsl::span<uint8_t>));

    MOCK_CONST_METHOD0(isInitiatorMock, bool(void));
    bool isInitiator() const noexcept override {
      return isInitiatorMock();
    }

    MOCK_METHOD0(localMultiaddr, outcome::result<multi::Multiaddress>(void));

    MOCK_METHOD0(remoteMultiaddr, outcome::result<multi::Multiaddress>(void));

    MOCK_CONST_METHOD0(localPeer, peer::PeerId(void));

    MOCK_CONST_METHOD0(remotePeer, peer::PeerId(void));

    MOCK_CONST_METHOD0(remotePublicKey, crypto::PublicKey(void));
  };
}  // namespace libp2p::connection

#endif  // KAGOME_SECURE_CONNECTION_MOCK_HPP
