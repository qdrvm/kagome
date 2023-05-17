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

    MOCK_METHOD(void,
                registerHandler,
                (const std::string &name, Method method, bool),
                (override));

    MOCK_METHOD(std::vector<std::string>, getHandlerNames, (), (override));

    MOCK_METHOD(void,
                processData,
                (std::string_view, bool, const ResponseHandler &),
                (override));

    MOCK_METHOD(void,
                processJsonData,
                (std::string,
                 const jsonrpc::Request::Parameters &,
                 const FormatterHandler &cb),
                (override));
  };

}  // namespace kagome::api

#endif  // KAGOME_MOCK_API_JRPC_SERVER_MOCK_HPP
