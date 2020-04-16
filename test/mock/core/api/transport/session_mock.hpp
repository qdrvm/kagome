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

#ifndef KAGOME_TEST_MOCK_API_TRANSPORT_SESSION_MOCK_HPP
#define KAGOME_TEST_MOCK_API_TRANSPORT_SESSION_MOCK_HPP

#include "api/transport/session.hpp"

namespace kagome::api {
  class SessionMock : public Session {
   public:
    ~SessionMock() override = default;
    MOCK_METHOD0(start, void());
    MOCK_METHOD1(respond, void(std::string_view));
  };
}  // namespace kagome::api

#endif  // KAGOME_TEST_MOCK_API_TRANSPORT_SESSION_MOCK_HPP
