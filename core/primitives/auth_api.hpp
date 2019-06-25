/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_PRIMITIVES_AUTH_API_HPP
#define KAGOME_CORE_PRIMITIVES_AUTH_API_HPP

#include <memory>
#include <optional>

#include <boost/variant.hpp>

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
   * @brief Metadata primitive
   */
  using Metadata = std::optional<std::shared_ptr<Session>>;

  /**
   * @brief Subscriber primitive
   */
  struct Subscriber {
    uint32_t id{};
  };

  /**
   * @brief ExtrinsicKey is used as a key to search extrinsic
   */
  using ExtrinsicKey = std::vector<uint8_t>;
  // TODO(yuraz): PRE-221 investigate and implement Subscriber primitive

}  // namespace kagome::primitives

#endif  // KAGOME_CORE_PRIMITIVES_AUTH_API_HPP
