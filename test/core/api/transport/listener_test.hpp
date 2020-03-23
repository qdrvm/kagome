/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TEST_CORE_API_TRANSPORT_LISTENER_TEST_HPP
#define KAGOME_TEST_CORE_API_TRANSPORT_LISTENER_TEST_HPP

#include "api/transport/listener.hpp"

#include <gtest/gtest.h>
#include "api/jrpc/jrpc_processor.hpp"
#include "api/jrpc/jrpc_server.hpp"
#include <api/service/api_service.hpp>
#include <core/api/client/http_client.hpp>
#include "mock/core/api/transport/api_stub.hpp"
#include "mock/core/api/transport/jrpc_processor_stub.hpp"
#include "transaction_pool/transaction_pool_error.hpp"

using kagome::api::ApiService;
using kagome::api::ApiStub;
using kagome::api::JRpcProcessor;
using kagome::api::JrpcProcessorStub;
using kagome::api::JRpcServer;
using kagome::api::JRpcServerImpl;
using kagome::api::Listener;

template <typename ListenerImpl,
          typename =
              std::enable_if_t<std::is_base_of_v<Listener, ListenerImpl>, void>>
struct ListenerTest : public ::testing::Test {
  template <class T>
  using sptr = std::shared_ptr<T>;

 protected:
  using Endpoint = boost::asio::ip::tcp::endpoint;
  using Context = boost::asio::io_context;
  using Socket = boost::asio::ip::tcp::socket;
  using Timer = boost::asio::steady_timer;
  using Streambuf = boost::asio::streambuf;
  using Duration = boost::asio::steady_timer::duration;
  using ErrorCode = boost::system::error_code;

  std::shared_ptr<Context> main_context = std::make_shared<Context>(1);
  std::shared_ptr<Context> client_context = std::make_shared<Context>(1);

  int64_t payload;
  std::string request;
  std::string response;

  void SetUp() override {
    //		main_context = std::make_shared<Context>(1);
    //		client_context = std::make_shared<Context>(1);
    payload = 0xABCDEF;

    request = R"({"jsonrpc":"2.0","method":"echo","id":0,"params":[)"
              + std::to_string(payload) + "]}";
    response =
        R"({"jsonrpc":"2.0","id":0,"result":)" + std::to_string(payload) + "}";
  }
  void TearDown() override {
    //		main_context.reset();
    //		client_context.reset();
    request.clear();
    response.clear();
  }

  Endpoint endpoint = {boost::asio::ip::address::from_string("127.0.0.1"),
                       4321};
  typename ListenerImpl::Session::Configuration config{};

  sptr<ApiStub> api = std::make_shared<ApiStub>();

  sptr<JRpcServer> server = std::make_shared<JRpcServerImpl>();

  std::vector<std::shared_ptr<JRpcProcessor>> processors{
      std::make_shared<JrpcProcessorStub>(server, api)};

  std::vector<std::shared_ptr<Listener>> listeners{
      std::make_shared<ListenerImpl>(
          *main_context,
          typename ListenerImpl::Configuration{endpoint},
          config)};

  sptr<ApiService> service = std::make_shared<ApiService>(
      std::vector<std::shared_ptr<Listener>>(listeners), server, processors);
};

#endif  // KAGOME_TEST_CORE_API_TRANSPORT_LISTENER_TEST_HPP
