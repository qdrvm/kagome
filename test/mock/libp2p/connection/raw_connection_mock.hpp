/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_RAW_CONNECTION_MOCK_HPP
#define KAGOME_RAW_CONNECTION_MOCK_HPP

#include "libp2p/connection/raw_connection.hpp"

#include <gmock/gmock.h>
#include "mock/libp2p/connection/connection_mock_common.hpp"

namespace libp2p::connection {

  class RawConnectionMock : public virtual RawConnection {
   public:
    MOCK_CONST_METHOD0(isClosed, bool(void));

    MOCK_METHOD0(close, outcome::result<void>(void));

    MOCK_METHOD1(read, outcome::result<std::vector<uint8_t>>(size_t));

    MOCK_METHOD1(readSome, outcome::result<std::vector<uint8_t>>(size_t));

    MOCK_METHOD1(read, outcome::result<size_t>(gsl::span<uint8_t>));

    MOCK_METHOD1(readSome, outcome::result<size_t>(gsl::span<uint8_t>));

    MOCK_METHOD1(write, outcome::result<size_t>(gsl::span<const uint8_t>));

    MOCK_METHOD1(writeSome, outcome::result<size_t>(gsl::span<const uint8_t>));

    bool isInitiator() const noexcept override {
      return isInitiator_hack();
    }

    MOCK_CONST_METHOD0(isInitiator_hack, bool());

    MOCK_METHOD0(localMultiaddr, outcome::result<multi::Multiaddress>());

    MOCK_METHOD0(remoteMultiaddr, outcome::result<multi::Multiaddress>());
  };

}  // namespace libp2p::connection

#endif  // KAGOME_RAW_CONNECTION_MOCK_HPP
