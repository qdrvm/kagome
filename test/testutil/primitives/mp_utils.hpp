/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/blob.hpp"

namespace testutil {
  /**
   * @brief creates hash from initializers list
   * @param bytes initializers
   * @return newly created hash
   */
  kagome::common::Hash256 createHash256(std::initializer_list<uint8_t> bytes);

}  // namespace testutil
