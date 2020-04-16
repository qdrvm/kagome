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

#ifndef KAGOME_CORE_PRIMITIVES_OPAQUE_METADATA_HPP
#define KAGOME_CORE_PRIMITIVES_OPAQUE_METADATA_HPP

#include <vector>
#include <cstdint>

namespace kagome::primitives {
  /**
   * Polkadot primitive, which is opaque representation of RuntimeMetadata
   */
  using OpaqueMetadata = std::vector<uint8_t>;
}

#endif //KAGOME_CORE_PRIMITIVES_OPAQUE_METADATA_HPP
