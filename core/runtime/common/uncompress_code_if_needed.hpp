/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/buffer.hpp"

namespace kagome::runtime {
  enum class UncompressError : uint8_t {
    ZSTD_ERROR,
    BOMB_SIZE_REACHED,
  };

  outcome::result<void> uncompressCodeIfNeeded(common::BufferView buf,
                                               common::Buffer &res);
}  // namespace kagome::runtime

OUTCOME_HPP_DECLARE_ERROR(kagome::runtime, UncompressError);
