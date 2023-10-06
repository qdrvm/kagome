/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_UNCOMPRESS_IF_NEEDED_HPP
#define KAGOME_CORE_RUNTIME_UNCOMPRESS_IF_NEEDED_HPP

#include "common/buffer.hpp"

namespace kagome::runtime {
  enum class UncompressError : uint8_t { ZSTD_ERROR };

  outcome::result<void> uncompressCodeIfNeeded(common::BufferView buf,
                                               common::Buffer &res);
}  // namespace kagome::runtime

OUTCOME_HPP_DECLARE_ERROR(kagome::runtime, UncompressError);

#endif  // KAGOME_CORE_RUNTIME_UNCOMPRESS_IF_NEEDED_HPP
