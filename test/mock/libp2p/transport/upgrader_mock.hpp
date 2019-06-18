/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_UPGRADER_MOCK_HPP
#define KAGOME_UPGRADER_MOCK_HPP

#include "libp2p/transport/upgrader.hpp"

#include <gmock/gmock.h>

namespace libp2p::transport {

  class UpgraderMock : public Upgrader {
   public:
    MOCK_METHOD2(upgradeToSecure,
                 void(Upgrader::RawSPtr, Upgrader::OnSecuredCallbackFunc));

    MOCK_METHOD2(upgradeToMuxed,
                 void(Upgrader::SecureSPtr, Upgrader::OnMuxedCallbackFunc));
  };

}  // namespace libp2p::transport

#endif  // KAGOME_UPGRADER_MOCK_HPP
