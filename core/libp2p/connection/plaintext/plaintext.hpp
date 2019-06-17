/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_PLAINTEXT_CONNECTION_HPP
#define KAGOME_PLAINTEXT_CONNECTION_HPP

#include <memory>

#include "libp2p/connection/secure_connection.hpp"

namespace libp2p::connection {
  class PlaintextConnection : public SecureConnection {
   public:
    /**
     * Create an instance of plaintext secure connection from the raw
     * connection; this connection just forwards all writes/reads without any
     * en/decryption
     * @param raw_connection, over which the security is to be established
     */
    explicit PlaintextConnection(std::shared_ptr<RawConnection> raw_connection);

    ~PlaintextConnection() override = default;

    enum class Error { FIELD_IS_UNSUPPORTED = 1 };

    outcome::result<peer::PeerId> localPeer() const override;

    outcome::result<peer::PeerId> remotePeer() const override;

    outcome::result<crypto::PublicKey> remotePublicKey() const override;

    bool isInitiator() const noexcept override;

    outcome::result<multi::Multiaddress> localMultiaddr() override;

    outcome::result<multi::Multiaddress> remoteMultiaddr() override;

    void read(gsl::span<uint8_t> out, ReadCallbackFunc cb) override;

    void readSome(gsl::span<uint8_t> out, ReadCallbackFunc cb) override;

    void write(gsl::span<const uint8_t> in, WriteCallbackFunc cb) override;

    void writeSome(gsl::span<const uint8_t> in, WriteCallbackFunc cb) override;

    bool isClosed() const override;

    outcome::result<void> close() override;

   private:
    std::shared_ptr<RawConnection> raw_connection_;
  };
}  // namespace libp2p::connection

OUTCOME_HPP_DECLARE_ERROR(libp2p::connection, PlaintextConnection::Error)

#endif  // KAGOME_PLAINTEXT_CONNECTION_HPP
