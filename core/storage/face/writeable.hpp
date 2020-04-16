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

#ifndef KAGOME_WRITEABLE_HPP
#define KAGOME_WRITEABLE_HPP

#include <outcome/outcome.hpp>

namespace kagome::storage::face {

  /**
   * @brief An mixin for modifiable map.
   * @tparam K key type
   * @tparam V value type
   */
  template <typename K, typename V>
  struct Writeable {
    virtual ~Writeable() = default;

    /**
     * @brief Store value by key
     * @param key key
     * @param value value
     * @return result containing void if put successful, error otherwise
     */
    virtual outcome::result<void> put(const K &key, const V &value) = 0;
    virtual outcome::result<void> put(const K &key, V&& value) = 0;

    /**
     * @brief Remove value by key
     * @param key K
     * @return error code if error happened
     */
    virtual outcome::result<void> remove(const K &key) = 0;
  };

}  // namespace kagome::storage::face

#endif  // KAGOME_WRITEABLE_HPP
