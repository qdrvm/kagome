/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "testutil/primitives/mp_utils.hpp"

namespace testutil {
  kagome::common::Hash256 createHash256(std::initializer_list<uint8_t> bytes) {
    kagome::common::Hash256 h;
    h.fill(0u);
    std::copy_n(bytes.begin(), bytes.size(), h.begin());
    return h;
  }
}  // namespace testutil
