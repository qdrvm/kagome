/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_READ_WRITER_MOCK_HPP
#define LIBP2P_READ_WRITER_MOCK_HPP

#include <libp2p/basic/readwriter.hpp>

#include <gmock/gmock.h>

namespace libp2p::basic {
  struct ReadWriterMock : public ReadWriter {
    MOCK_METHOD(void,
                read,
                (gsl::span<uint8_t>, size_t, Reader::ReadCallbackFunc),
                (override));

    MOCK_METHOD(void,
                readSome,
                (gsl::span<uint8_t>, size_t, Reader::ReadCallbackFunc),
                (override));

    MOCK_METHOD(void,
                write,
                (gsl::span<const uint8_t>, size_t, Writer::WriteCallbackFunc),
                (override));

    MOCK_METHOD(void,
                writeSome,
                (gsl::span<const uint8_t>, size_t, Writer::WriteCallbackFunc),
                (override));

    MOCK_METHOD(void,
                deferReadCallback,
                (outcome::result<size_t>, Reader::ReadCallbackFunc),
                (override));

    MOCK_METHOD(void,
                deferWriteCallback,
                (std::error_code, Writer::WriteCallbackFunc),
                (override));
  };
}  // namespace libp2p::basic

#endif  // LIBP2P_READ_WRITER_MOCK_HPP
