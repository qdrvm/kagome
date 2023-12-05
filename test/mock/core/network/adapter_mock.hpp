/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

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

    MOCK_METHOD(size_t, m_size, (const Dummy &), ());

    MOCK_METHOD(BufferIt, m_write, (const Dummy &, Buffer &, BufferIt), ());

    MOCK_METHOD(Result, m_read, (Dummy &, const Buffer &, BufferCIt), ());
  };

}  // namespace kagome::network
