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

#include "testutil/primitives/mp_utils.hpp"

namespace testutil {
  kagome::common::Hash256 createHash256(std::initializer_list<uint8_t> bytes) {
    kagome::common::Hash256 h;
    h.fill(0u);
    std::copy_n(bytes.begin(), bytes.size(), h.begin());
    return h;
  }
}  // namespace testutil
