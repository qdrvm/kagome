/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <tuple>

namespace kagome {

  /// Special zero-size-type for some things
  /// (e.g. unsupported, experimental or empty).
  using Empty = std::tuple<>;

}  // namespace kagome
