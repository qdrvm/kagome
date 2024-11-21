/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <memory>

#include "scale/kagome_scale.hpp"

#include "common/buffer.hpp"

namespace kagome::network::notifications {
  std::shared_ptr<Buffer> encode(const auto &message) {
    return std::make_shared<Buffer>(scale::encode(message).value());
  }
}  // namespace kagome::network::notifications
