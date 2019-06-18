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
  using SubscriptionId = boost::variant<uint64_t, std::string>;

  /**
   * @brief Session primitive
   */
  class Session;
  // TODO(yuraz): investigate Session primitive

  /**
   * @brief Metadata primitive
   */
  using Metadata = std::optional<std::shared_ptr<Session>>;

  /**
   * @brief Subscriber primitive
   */
  class Subscriber;
  // TODO(yuraz): investigate Subscriber primitive

}  // namespace kagome::primitives

#endif  // KAGOME_CORE_PRIMITIVES_AUTH_API_HPP
