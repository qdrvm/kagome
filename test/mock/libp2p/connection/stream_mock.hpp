/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_STREAM_MOCK_HPP
#define KAGOME_STREAM_MOCK_HPP

#include <gmock/gmock.h>
#include "libp2p/connection/stream.hpp"

namespace libp2p::connection {
  struct StreamMock : public Stream {
    StreamMock() = default;
    StreamMock(uint8_t id) : stream_id{id} {}

    /// this field here is for easier testing
    uint8_t stream_id = 0;

    ~StreamMock() override = default;

    MOCK_CONST_METHOD0(isClosed, bool(void));

    MOCK_METHOD0(close, outcome::result<void>(void));

    MOCK_METHOD1(read, outcome::result<std::vector<uint8_t>>(size_t));

    MOCK_METHOD1(readSome, outcome::result<std::vector<uint8_t>>(size_t));

    MOCK_METHOD1(read, outcome::result<size_t>(gsl::span<uint8_t>));

    MOCK_METHOD1(readSome, outcome::result<size_t>(gsl::span<uint8_t>));

    MOCK_METHOD1(write, outcome::result<size_t>(gsl::span<const uint8_t>));

    MOCK_METHOD1(writeSome, outcome::result<size_t>(gsl::span<const uint8_t>));

    MOCK_METHOD0(reset, void());

    MOCK_CONST_METHOD0(isClosedForRead, bool());

    MOCK_CONST_METHOD0(isClosedForWrite, bool());
  };
}  // namespace libp2p::connection

#endif  // KAGOME_STREAM_MOCK_HPP
