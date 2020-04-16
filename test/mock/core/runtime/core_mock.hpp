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

#ifndef KAGOME_TEST_MOCK_CORE_RUNTIME_CORE_MOCK_HPP
#define KAGOME_TEST_MOCK_CORE_RUNTIME_CORE_MOCK_HPP

#include "runtime/core.hpp"

#include <gmock/gmock.h>

namespace kagome::runtime {
  class CoreMock : public Core {
   public:
    MOCK_METHOD0(version, outcome::result<primitives::Version>());
    MOCK_METHOD1(execute_block,
                 outcome::result<void>(const primitives::Block &));
    MOCK_METHOD1(initialise_block,
                 outcome::result<void>(const primitives::BlockHeader &));
    MOCK_METHOD1(authorities,
                 outcome::result<std::vector<primitives::AuthorityId>>(
                     const primitives::BlockId &));
  };
}  // namespace kagome::runtime

#endif  // KAGOME_TEST_MOCK_CORE_RUNTIME_CORE_MOCK_HPP
