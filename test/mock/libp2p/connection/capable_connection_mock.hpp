/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CAPABLE_CONNECTION_MOCK_HPP
#define KAGOME_CAPABLE_CONNECTION_MOCK_HPP

#include <gmock/gmock.h>

#include "libp2p/connection/capable_connection.hpp"

#include "mock/libp2p/connection/connection_mock_common.hpp"

namespace libp2p::connection {

  class CapableConnectionMock : public CapableConnection {
   public:
    MOCK_METHOD0(newStream, outcome::result<std::shared_ptr<Stream>>());

    MOCK_CONST_METHOD0(localPeer, outcome::result<peer::PeerId>());
    MOCK_CONST_METHOD0(remotePeer, outcome::result<peer::PeerId>());
    MOCK_CONST_METHOD0(remotePublicKey, outcome::result<crypto::PublicKey>());

    MOCK_CONST_METHOD0(isClosed, bool(void));
    MOCK_METHOD0(close, outcome::result<void>(void));
    MOCK_METHOD1(read, outcome::result<std::vector<uint8_t>>(size_t));
    MOCK_METHOD1(readSome, outcome::result<std::vector<uint8_t>>(size_t));
    MOCK_METHOD1(read, outcome::result<size_t>(gsl::span<uint8_t>));
    MOCK_METHOD1(readSome, outcome::result<size_t>(gsl::span<uint8_t>));
    MOCK_METHOD1(write, outcome::result<size_t>(gsl::span<const uint8_t>));
    MOCK_METHOD1(writeSome, outcome::result<size_t>(gsl::span<const uint8_t>));
    bool isInitiator() const noexcept override {
      return isInitiator_hack();
    }
    MOCK_CONST_METHOD0(isInitiator_hack, bool());
    MOCK_METHOD0(localMultiaddr, outcome::result<multi::Multiaddress>());
    MOCK_METHOD0(remoteMultiaddr, outcome::result<multi::Multiaddress>());
  };

  class CapableConnBasedOnRawConnMock : public CapableConnection {
   public:
    explicit CapableConnBasedOnRawConnMock(std::shared_ptr<RawConnection> c)
        : real_(std::move(c)) {}

    MOCK_METHOD0(newStream, outcome::result<std::shared_ptr<Stream>>());

    MOCK_CONST_METHOD0(localPeer, outcome::result<peer::PeerId>());

    MOCK_CONST_METHOD0(remotePeer, outcome::result<peer::PeerId>());

    MOCK_CONST_METHOD0(remotePublicKey, outcome::result<crypto::PublicKey>());

    bool isInitiator() const noexcept override {
      return real_->isInitiator();
    }

    outcome::result<multi::Multiaddress> localMultiaddr() override {
      return real_->localMultiaddr();
    }

    outcome::result<multi::Multiaddress> remoteMultiaddr() override {
      return real_->remoteMultiaddr();
    };

    outcome::result<std::vector<uint8_t>> read(size_t bytes) override {
      return real_->read(bytes);
    };

    outcome::result<std::vector<uint8_t>> readSome(size_t bytes) override {
      return real_->readSome(bytes);
    }

    outcome::result<size_t> read(gsl::span<uint8_t> buf) override {
      return real_->read(buf);
    };

    outcome::result<size_t> readSome(gsl::span<uint8_t> buf) override {
      return real_->readSome(buf);
    };

    outcome::result<size_t> write(gsl::span<const uint8_t> in) override {
      return real_->write(in);
    };

    outcome::result<size_t> writeSome(gsl::span<const uint8_t> in) override {
      return real_->writeSome(in);
    };

    bool isClosed() const override {
      return real_->isClosed();
    };

    outcome::result<void> close() override {
      return real_->close();
    };

   private:
    std::shared_ptr<RawConnection> real_;
  };
}  // namespace libp2p::connection

#endif  // KAGOME_CAPABLE_CONNECTION_MOCK_HPP
