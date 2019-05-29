/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_PLAINTEXT_ADAPTOR_HPP
#define KAGOME_PLAINTEXT_ADAPTOR_HPP

#include "libp2p/security/security_adaptor.hpp"

namespace libp2p::security {
  class PlaintextAdaptor : public SecurityAdaptor {
   public:
    ~PlaintextAdaptor() override = default;

    const peer::Protocol &getProtocolId() const override;

    outcome::result<std::shared_ptr<connection::SecureConnection>>
    secureInbound(std::shared_ptr<connection::RawConnection> inbound) override;

    outcome::result<std::shared_ptr<connection::SecureConnection>>
    secureOutbound(std::shared_ptr<connection::RawConnection> outbound,
                   const peer::PeerId &p) override;

   private:
    // TODO(akvinikym) 29.05.19: think about creating SecurityProtocolRegister
    static const peer::Protocol kProtocolId;
  };
}  // namespace libp2p::security

#endif  // KAGOME_PLAINTEXT_ADAPTOR_HPP
