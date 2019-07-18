/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_DIALER_MOCK_HPP
#define KAGOME_DIALER_MOCK_HPP

#include <gmock/gmock.h>
#include "libp2p/network/dialer.hpp"

namespace libp2p::network {

  struct DialerMock : public Dialer {
    ~DialerMock() override = default;

    MOCK_METHOD2(dial, void(const peer::PeerInfo &, DialResultFunc));
    MOCK_METHOD3(newStream,
                 void(const peer::PeerInfo &, const peer::Protocol &,
                      StreamResultFunc));
  };

}  // namespace libp2p::network

#endif  // KAGOME_DIALER_MOCK_HPP
