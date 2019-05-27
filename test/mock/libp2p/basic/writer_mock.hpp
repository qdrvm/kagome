/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_WRITER_MOCK_HPP
#define KAGOME_WRITER_MOCK_HPP

#include <gmock/gmock.h>
#include "libp2p/basic/writer.hpp"

namespace libp2p::basic {
  class WriterMock : public Writer {
   public:
    ~WriterMock() override = default;

    MOCK_METHOD1(write, outcome::result<size_t>(gsl::span<const uint8_t>));

    MOCK_METHOD1(writeSome, outcome::result<size_t>(gsl::span<const uint8_t>));
  };
}  // namespace libp2p::basic

#endif  // KAGOME_WRITER_MOCK_HPP
