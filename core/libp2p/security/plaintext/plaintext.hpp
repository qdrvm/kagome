/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_PLAINTEXT_ADAPTOR_HPP
#define KAGOME_PLAINTEXT_ADAPTOR_HPP

#include "libp2p/crypto/key_marshaller.hpp"
#include "libp2p/peer/identity_manager.hpp"
#include "libp2p/security/security_adaptor.hpp"

namespace libp2p::security {
  /**
   * Implementation of security adaptor, which creates plaintext connection.
   *
   * Protocol is the following:
   * 1. Initiator immediately sends his public key to the other peer.
   * 2. Responder receives public key, saves it
   * 3. Responder answers with his public key
   * 4. Initiator calculates PeerId from responder's public key, if it differs
   * from the one supplied in dial, AUTHENTICATION_ERROR
   */
  class Plaintext : public SecurityAdaptor {
   public:
    ~Plaintext() override = default;

    Plaintext(std::shared_ptr<crypto::marshaller::KeyMarshaller> marshaller,
              std::shared_ptr<peer::IdentityManager> idmgr);

    peer::Protocol getProtocolId() const override;

    void secureInbound(std::shared_ptr<connection::RawConnection> inbound,
                       SecConnCallbackFunc cb) override;

    void secureOutbound(std::shared_ptr<connection::RawConnection> outbound,
                        const peer::PeerId &p, SecConnCallbackFunc cb) override;

   private:
    std::shared_ptr<crypto::marshaller::KeyMarshaller> marshaller_;
    std::shared_ptr<peer::IdentityManager> idmgr_;
  };
}  // namespace libp2p::security

#endif  // KAGOME_PLAINTEXT_ADAPTOR_HPP
