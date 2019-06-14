/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_STREAM_MOCK_HPP
#define KAGOME_STREAM_MOCK_HPP

#include <gmock/gmock.h>
#include "libp2p/connection/stream.hpp"
#include "mock/libp2p/connection/connection_mock_common.hpp"

namespace libp2p::connection {
  class StreamMock : public Stream {
   public:
    StreamMock() = default;
    StreamMock(uint8_t id) : stream_id{id} {}

    /// this field here is for easier testing
    uint8_t stream_id = 137;

    ~StreamMock() override = default;

    MOCK_CONST_METHOD0(isClosed, bool(void));

    MOCK_METHOD0(close, cti::continuable<>(void));

    MOCK_METHOD1(write, cti::continuable<size_t>(gsl::span<const uint8_t>));

    MOCK_METHOD1(writeSome, cti::continuable<size_t>(gsl::span<const uint8_t>));

    MOCK_METHOD1(read, cti::continuable<std::vector<uint8_t>>(size_t));

    MOCK_METHOD1(readSome, cti::continuable<std::vector<uint8_t>>(size_t));

    MOCK_METHOD0(reset, cti::continuable<>(void));

    MOCK_CONST_METHOD0(isClosedForRead, bool(void));

    MOCK_CONST_METHOD0(isClosedForWrite, bool(void));

    MOCK_METHOD1(adjustWindowSize, cti::continuable<>(uint32_t));
  };
}  // namespace libp2p::connection

#endif  // KAGOME_STREAM_MOCK_HPP
