/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <algorithm>
#include <unordered_set>
#include <vector>

namespace kagome {
  template <typename T>
  void retain_if(std::vector<T> &v, auto &&predicate) {
    v.erase(std::remove_if(
                v.begin(), v.end(), [&](T &v) { return not predicate(v); }),
            v.end());
  }
  template <typename C>
    requires requires { typename C::key_type; }
  void retain_if(C &v, auto &&predicate) {
    for (auto it = v.begin(); it != v.end();) {
      if (!predicate(*it)) {
        it = v.erase(it);
      } else {
        ++it;
      }
    }
  }

}  // namespace kagome
