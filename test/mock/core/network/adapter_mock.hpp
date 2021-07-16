/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TEST_CORE_NETWORK_ADAPTER_MOCK_HPP
#define KAGOME_TEST_CORE_NETWORK_ADAPTER_MOCK_HPP

#include <gmock/gmock.h>

#include "outcome/outcome.hpp"

namespace kagome::network {

  struct Dummy {};
  bool operator==(const Dummy &d_0, const Dummy &d_1) {
    return true;  //&d_0 == &d_1;
  }

  class AdapterMock final {
   public:
    using Buffer = std::vector<uint8_t>;
    using BufferIt = Buffer::iterator;
    using BufferCIt = Buffer::const_iterator;
    using Result = outcome::result<BufferCIt>;

    ~AdapterMock() = default;

    MOCK_METHOD1(m_size, size_t(const Dummy &));
    MOCK_METHOD3(m_write, BufferIt(const Dummy &, Buffer &, BufferIt));
    MOCK_METHOD3(m_read, Result(Dummy &, const Buffer &, BufferCIt));
  };

}  // namespace kagome::network

#endif  // KAGOME_TEST_CORE_NETWORK_ADAPTER_MOCK_HPP
