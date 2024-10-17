/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <cstdint>
#include <span>
#include <vector>

#include "outcome/outcome.hpp"

namespace kagome::network {
struct ICompressor {
  virtual ~ICompressor() = default;
  virtual outcome::result<std::vector<uint8_t>> compress(std::span<uint8_t> data) = 0;
  virtual outcome::result<std::vector<uint8_t>> decompress(std::span<uint8_t> compressedData) = 0;
};

}  // namespace kagome::network
