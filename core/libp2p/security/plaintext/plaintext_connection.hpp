/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_PLAINTEXT_CONNECTION_HPP
#define KAGOME_PLAINTEXT_CONNECTION_HPP

#include <memory>
#include <optional>

#include "libp2p/connection/secure_connection.hpp"

namespace libp2p::connection {
  class PlaintextConnection : public SecureConnection {
   public:
    PlaintextConnection(std::shared_ptr<RawConnection> raw_connection,
                        crypto::PublicKey localPubkey,
                        crypto::PublicKey remotePubkey);

    ~PlaintextConnection() override = default;

    outcome::result<peer::PeerId> localPeer() const override;

    outcome::result<peer::PeerId> remotePeer() const override;

    outcome::result<crypto::PublicKey> remotePublicKey() const override;

    bool isInitiator() const noexcept override;

    outcome::result<multi::Multiaddress> localMultiaddr() override;

    outcome::result<multi::Multiaddress> remoteMultiaddr() override;

    void read(gsl::span<uint8_t> out, size_t bytes,
              ReadCallbackFunc cb) override;

    void readSome(gsl::span<uint8_t> out, size_t bytes,
                  ReadCallbackFunc cb) override;

    void write(gsl::span<const uint8_t> in, size_t bytes,
               WriteCallbackFunc cb) override;

    void writeSome(gsl::span<const uint8_t> in, size_t bytes,
                   WriteCallbackFunc cb) override;

    bool isClosed() const override;

    outcome::result<void> close() override;

   private:
    std::shared_ptr<RawConnection> raw_connection_;

    crypto::PublicKey local_;
    crypto::PublicKey remote_;
  };
}  // namespace libp2p::connection

#endif  // KAGOME_PLAINTEXT_CONNECTION_HPP
