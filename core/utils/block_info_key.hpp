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
  struct BlockInfoKey {
    static constexpr size_t kNumberSize = sizeof(primitives::BlockNumber);
    static constexpr size_t kHashSize = primitives::BlockHash::size();
    using Key = common::Blob<kNumberSize + kHashSize>;

    static Key encode(const primitives::BlockInfo &info) {
      Key key;
      boost::endian::store_big_u32(key.data(), info.number);
      std::copy_n(info.hash.data(), kHashSize, key.data() + kNumberSize);
      return key;
    }

    static std::optional<primitives::BlockInfo> decode(common::BufferView key) {
      if (key.size() != Key::size()) {
        return std::nullopt;
      }
      primitives::BlockInfo info;
      info.number = boost::endian::load_big_u32(key.data());
      std::copy_n(key.data() + kNumberSize, kHashSize, info.hash.data());
      return info;
    }
  };
}  // namespace kagome
