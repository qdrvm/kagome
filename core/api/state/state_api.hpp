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

#ifndef KAGOME_API_STATE_API_HPP
#define KAGOME_API_STATE_API_HPP

#include "common/buffer.hpp"
#include "outcome/outcome.hpp"
#include "primitives/common.hpp"

namespace kagome::api {

  class StateApi {
   public:
    virtual ~StateApi() = default;
    virtual outcome::result<common::Buffer> getStorage(
        const common::Buffer &key) const = 0;
    virtual outcome::result<common::Buffer> getStorage(
        const common::Buffer &key, const primitives::BlockHash &at) const = 0;
  };

}  // namespace kagome::api

#endif  // KAGOME_API_STATE_API_HPP
