/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_SECURITY_ADAPTOR_MOCK_HPP
#define KAGOME_SECURITY_ADAPTOR_MOCK_HPP

#include <gmock/gmock.h>
#include "libp2p/security/security_adaptor.hpp"

namespace libp2p::security {
  struct SecurityAdaptorMock : public SecurityAdaptor {
   public:
    MOCK_CONST_METHOD0(getProtocolId, peer::Protocol());

    MOCK_METHOD1(secureInbound,
                 outcome::result<std::shared_ptr<connection::SecureConnection>>(
                     std::shared_ptr<connection::RawConnection>));

    MOCK_METHOD2(secureOutbound,
                 outcome::result<std::shared_ptr<connection::SecureConnection>>(
                     std::shared_ptr<connection::RawConnection>,
                     const peer::PeerId &));
  };
}  // namespace libp2p::security

#endif  // KAGOME_SECURITY_ADAPTOR_MOCK_HPP
