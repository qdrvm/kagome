/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <algorithm>
#include <vector>

namespace kagome {
  template <typename T>
  void retain(std::vector<T> &v, auto &&predicate) {
    v.erase(std::remove_if(
                v.begin(), v.end(), [&](T &v) { return not predicate(v); }),
            v.end());
  }
}  // namespace kagome
