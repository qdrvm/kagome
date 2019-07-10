/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/security/plaintext/plaintext.hpp"

#include "libp2p/security/error.hpp"
#include "libp2p/security/plaintext/plaintext_connection.hpp"
#include "libp2p/security/plaintext/plaintext_session.hpp"

namespace libp2p::security {
  peer::Protocol Plaintext::getProtocolId() const {
    // TODO(akvinikym) 29.05.19: think about creating SecurityProtocolRegister
    return "/plaintext/1.0.0";
  }

  void Plaintext::secureInbound(
      std::shared_ptr<connection::RawConnection> inbound,
      SecConnCallbackFunc cb) {
    auto session =
        std::make_shared<PlaintextSession>(this->marshaller_, inbound, cb);

    // wait until the initiator sends us their public key
    session->recvKey([this, cb{std::move(cb)}, inbound{std::move(inbound)},
                      session](crypto::PublicKey remoteKey) mutable {
      // when we received remote public key, send them our public key
      session->sendKey(
          this->idmgr_->getKeyPair().publicKey,
          [this, cb{std::move(cb)}, inbound{std::move(inbound)}, session,
           remoteKey{std::move(remoteKey)}]() mutable {
            auto plaintext = std::make_shared<connection::PlaintextConnection>(
                inbound, this->idmgr_->getKeyPair().publicKey, remoteKey);

            return cb(std::move(plaintext));
          });
    });
  }

  void Plaintext::secureOutbound(
      std::shared_ptr<connection::RawConnection> outbound,
      const peer::PeerId &p, SecConnCallbackFunc cb) {
    auto session =
        std::make_shared<PlaintextSession>(this->marshaller_, outbound, cb);

    // immediately send our public key
    session->sendKey(
        this->idmgr_->getKeyPair().publicKey,
        [this, session, outbound{std::move(outbound)}, p,
         cb{std::move(cb)}]() mutable {
          // then wait for key from other side
          session->recvKey([this, p, outbound{std::move(outbound)},
                            cb{std::move(cb)}](
                               crypto::PublicKey remotePubkey) mutable {
            auto remoteIdCalculated = peer::PeerId::fromPublicKey(remotePubkey);
            if (remoteIdCalculated != p) {
              return cb(make_error_code(SecurityError::AUTHENTICATION_ERROR));
            }

            auto plaintext = std::make_shared<connection::PlaintextConnection>(
                outbound, this->idmgr_->getKeyPair().publicKey, remotePubkey);

            return cb(std::move(plaintext));
          });
        });
  }

  Plaintext::Plaintext(
      std::shared_ptr<crypto::marshaller::KeyMarshaller> marshaller,
      std::shared_ptr<peer::IdentityManager> idmgr)
      : marshaller_(std::move(marshaller)), idmgr_(std::move(idmgr)) {}

}  // namespace libp2p::security
