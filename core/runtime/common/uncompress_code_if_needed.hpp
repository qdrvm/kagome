/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/buffer.hpp"
#include "outcome/custom.hpp"

namespace kagome::runtime {
  enum class UncompressError : uint8_t {
    ZSTD_ERROR,
    BOMB_SIZE_REACHED,
  };
}  // namespace kagome::runtime

OUTCOME_HPP_DECLARE_ERROR(kagome::runtime, UncompressError);

namespace kagome::runtime {
  inline auto format_as(UncompressError e) {
    return make_error_code(e);
  }
  template <typename R>
  using UncompressOutcome = CustomOutcome<R, UncompressError>;

  UncompressOutcome<void> uncompressCodeIfNeeded(common::BufferView buf,
                                                 common::Buffer &res);

  inline UncompressOutcome<Buffer> uncompressCodeIfNeeded(
      BufferView data_zstd) {
    Buffer data;
    OUTCOME_TRY(uncompressCodeIfNeeded(data_zstd, data));
    return data;
  }
}  // namespace kagome::runtime
