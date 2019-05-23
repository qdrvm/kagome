/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_SECURITY_IMPL_HPP
#define KAGOME_SECURITY_IMPL_HPP

#include "libp2p/security/security_adaptor.hpp"

namespace libp2p::security {
  /// stub for implementation of security adaptor, remove or use as place, where
  /// the adaptor will be implemented
  class SecurityImpl : public SecurityAdaptor {
   public:
    ~SecurityImpl() override = default;

    const peer::Protocol &getProtocolId() const override;

    outcome::result<std::shared_ptr<connection::SecureConnection>>
    secureInbound(std::shared_ptr<connection::RawConnection> inbound) override;

    outcome::result<std::shared_ptr<connection::SecureConnection>>
    secureOutbound(std::shared_ptr<connection::RawConnection> outbound,
                   const peer::PeerId &p) override;
  };
}  // namespace libp2p::security

#endif  // KAGOME_SECURITY_IMPL_HPP
