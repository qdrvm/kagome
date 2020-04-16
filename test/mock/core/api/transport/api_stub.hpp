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

#ifndef KAGOME_TEST_MOCK_API_API_STUB_HPP
#define KAGOME_TEST_MOCK_API_API_STUB_HPP

#include <gmock/gmock.h>

#include "outcome/outcome.hpp"

namespace kagome::api {

/**
 * @brief MPV implementation of possible API for using in tests instead real powerful API
 */
  class ApiStub {
   public:
    outcome::result<int64_t> echo(int64_t payload) {
      return payload;
    }
  };

}  // namespace kagome::api

#endif  // KAGOME_TEST_MOCK_API_API_STUB_HPP
