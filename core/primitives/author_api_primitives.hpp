/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <memory>

#include <boost/variant.hpp>
#include <optional>

/**
 * Authoring api primitives
 */

namespace kagome::primitives {
  /**
   * @brief SubscriptionId primitive
   */
  using SubscriptionId = uint64_t;

  /**
   * @brief Session primitive
   */
  struct Session {
    uint32_t id{};
  };
  // TODO(yuraz): PRE-221 investigate and implement Session primitive

  /**
   * @brief ExtrinsicKey is used as a key to search extrinsic
   */
  using ExtrinsicKey = std::vector<uint8_t>;

}  // namespace kagome::primitives
