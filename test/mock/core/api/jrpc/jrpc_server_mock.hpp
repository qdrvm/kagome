/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
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
    MOCK_METHOD2(processJsonData,
                 void(jsonrpc::Value const &from, ResponseHandler const &cb));
  };

}  // namespace kagome::api

#endif  // KAGOME_MOCK_API_JRPC_SERVER_MOCK_HPP
