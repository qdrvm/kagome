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

#ifndef KAGOME_TEST_CORE_API_JRPC_SERVER_MOCK_HPP
#define KAGOME_TEST_CORE_API_JRPC_SERVER_MOCK_HPP

#include <gmock/gmock.h>

#include "api/jrpc/jrpc_server.hpp"

namespace kagome::api {

  class JRpcServerMock : public JRpcServer {
   public:
    ~JRpcServerMock() override = default;

    MOCK_METHOD2(registerHandler, void(const std::string &name, Method method));
    MOCK_METHOD2(processData,
                 void(std::string_view request, const ResponseHandler &cb));
  };

}  // namespace kagome::api

#endif  // KAGOME_MOCK_API_JRPC_SERVER_MOCK_HPP
