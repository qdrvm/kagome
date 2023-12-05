/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <optional>

#include <boost/endian/conversion.hpp>

#include "primitives/common.hpp"

namespace kagome {

  struct BlockNumberKey {
    using Key = common::Blob<sizeof(primitives::BlockNumber)>;

    static Key encode(primitives::BlockNumber number) {
      Key key;
      boost::endian::store_big_u32(key.data(), number);
      return key;
    }

    static std::optional<primitives::BlockNumber> decode(
        common::BufferView key) {
      if (key.size() != Key::size()) {
        return std::nullopt;
      }
      return boost::endian::load_big_u32(key.data());
    }
  };

}  // namespace kagome
