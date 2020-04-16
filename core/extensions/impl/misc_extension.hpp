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

#ifndef KAGOME_MISC_EXTENSION_HPP
#define KAGOME_MISC_EXTENSION_HPP

#include <cstdint>

namespace kagome::extensions {
  /**
   * Implements miscellaneous extension functions
   */
  class MiscExtension {
   public:
    MiscExtension() = default;
    ~MiscExtension() = default;
    explicit MiscExtension(uint64_t chain_id);

    /**
     * @return id (a 64-bit unsigned integer) of the current chain
     */
    uint64_t ext_chain_id() const;

   private:
    const uint64_t chain_id_ = 42;
  };
}  // namespace kagome::extensions

#endif  // KAGOME_MISC_EXTENSION_HPP
