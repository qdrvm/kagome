/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_READER_MOCK_HPP
#define KAGOME_READER_MOCK_HPP

#include <gmock/gmock.h>
#include "common/hexutil.hpp"
#include "libp2p/basic/reader.hpp"

namespace libp2p::basic {
  class ReaderMock : public Reader {
   public:
    ~ReaderMock() override = default;

    MOCK_METHOD1(read, outcome::result<std::vector<uint8_t>>(size_t));

    MOCK_METHOD1(readSome, outcome::result<std::vector<uint8_t>>(size_t));

    MOCK_METHOD1(read, outcome::result<size_t>(gsl::span<uint8_t>));

    MOCK_METHOD1(readSome, outcome::result<size_t>(gsl::span<uint8_t>));
  };
}  // namespace libp2p::basic

inline std::ostream &operator<<(std::ostream &s,
                                const std::vector<unsigned char> &v) {
  s << kagome::common::hex_upper(v) << "\n";
  return s;
}

#endif  // KAGOME_READER_MOCK_HPP
