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

#ifndef KAGOME_CORE_PRIMITIVES_SESSION_KEY_HPP
#define KAGOME_CORE_PRIMITIVES_SESSION_KEY_HPP

#include "common/blob.hpp"

namespace kagome::primitives {
  // TODO(akvinikym): must be a SR25519 key
  using SessionKey = common::Blob<32>;
}  // namespace kagome::primitives

#endif  // KAGOME_CORE_PRIMITIVES_SESSION_KEY_HPP
