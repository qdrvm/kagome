/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 *
 * Kagome is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * Kagome is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef KAGOME_CORE_PRIMITIVES_EXTRINSIC_API_PRIMITIVES_HPP
#define KAGOME_CORE_PRIMITIVES_EXTRINSIC_API_PRIMITIVES_HPP

#include <memory>

#include <boost/optional.hpp>
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
  using Metadata = boost::optional<std::shared_ptr<Session>>;

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

#endif  //KAGOME_CORE_PRIMITIVES_EXTRINSIC_API_PRIMITIVES_HPP
