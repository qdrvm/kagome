/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_PLAINTEXT_ADAPTOR_HPP
#define KAGOME_PLAINTEXT_ADAPTOR_HPP

#include "libp2p/security/security_adaptor.hpp"

namespace libp2p::security {
  /**
   * Implementation of security adaptor, which created a plaintext connection
   */
  class Plaintext : public SecurityAdaptor {
   public:
    ~Plaintext() override = default;

    peer::Protocol getProtocolId() const override;

    outcome::result<std::shared_ptr<connection::SecureConnection>>
    secureInbound(std::shared_ptr<connection::RawConnection> inbound) override;

    outcome::result<std::shared_ptr<connection::SecureConnection>>
    secureOutbound(std::shared_ptr<connection::RawConnection> outbound,
                   const peer::PeerId &p) override;
  };
}  // namespace libp2p::security

#endif  // KAGOME_PLAINTEXT_ADAPTOR_HPP
