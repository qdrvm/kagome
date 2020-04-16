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

#ifndef KAGOME_TEST_MOCK_API_TRANSPORT_LISTENER_MOCK_HPP
#define KAGOME_TEST_MOCK_API_TRANSPORT_LISTENER_MOCK_HPP

#include <gmock/gmock.h>
#include "api/transport/listener.hpp"

namespace kagome::api {

  class ListenerMock : public Listener {
   public:
    ~ListenerMock() override = default;
    MOCK_METHOD1(start, void(NewSessionHandler));
    MOCK_METHOD0(stop, void(void));
    MOCK_METHOD1(acceptOnce, void(NewSessionHandler));
  };

}  // namespace kagome::api

#endif  // KAGOME_TEST_MOCK_API_TRANSPORT_LISTENER_MOCK_HPP
