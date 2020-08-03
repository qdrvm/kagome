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
#include "api/service/api_service.hpp"
#include "application/impl/app_state_manager_impl.hpp"
#include "common/buffer.hpp"
#include "core/api/client/http_client.hpp"
#include "mock/core/api/transport/api_stub.hpp"
#include "mock/core/api/transport/jrpc_processor_stub.hpp"
#include "mock/core/application/app_state_manager_mock.hpp"
#include "subscription/subscriber.hpp"
#include "transaction_pool/transaction_pool_error.hpp"

using namespace kagome::api;
using namespace kagome::common;
using namespace kagome::subscription;
using namespace kagome::primitives;

template <typename ListenerImpl,
          typename =
              std::enable_if_t<std::is_base_of_v<Listener, ListenerImpl>, void>>
struct ListenerTest : public ::testing::Test {
  template <class T>
  using sptr = std::shared_ptr<T>;

 protected:
  using Endpoint = boost::asio::ip::tcp::endpoint;
  using Context = RpcContext;
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
    payload = 0xABCDEF;

    request = R"({"jsonrpc":"2.0","method":"echo","id":0,"params":[)"
              + std::to_string(payload) + "]}";
    response =
        R"({"jsonrpc":"2.0","id":0,"result":)" + std::to_string(payload) + "}";
  }
  void TearDown() override {
    request.clear();
    response.clear();
  }

  typename ListenerImpl::Configuration listener_config = [] {
    typename ListenerImpl::Configuration config{};
    config.endpoint.address(boost::asio::ip::address::from_string("127.0.0.1"));
    config.endpoint.port(4321);
    return config;
  }();

  typename ListenerImpl::SessionImpl::Configuration session_config{};

  sptr<kagome::application::AppStateManager> app_state_manager =
      std::make_shared<kagome::application::AppStateManagerImpl>();

  kagome::api::RpcThreadPool::Configuration config = {1, 1};

  sptr<kagome::api::RpcThreadPool> thread_pool =
      std::make_shared<kagome::api::RpcThreadPool>(main_context, config);

  sptr<ApiStub> api = std::make_shared<ApiStub>();

  sptr<JRpcServer> server = std::make_shared<JRpcServerImpl>();

  std::vector<std::shared_ptr<JRpcProcessor>> processors{
      std::make_shared<JrpcProcessorStub>(server, api)};

  std::shared_ptr<Listener> listener = std::make_shared<ListenerImpl>(
      app_state_manager, main_context, listener_config, session_config);

  using SessionPtr = std::shared_ptr<Session>;
  using SubscriptionEngineType =
      SubscriptionEngine<Buffer, SessionPtr, Buffer, BlockHash>;
  std::shared_ptr<SubscriptionEngineType> subscription_engine =
      std::make_shared<SubscriptionEngineType>();

  sptr<ApiService> service = std::make_shared<ApiService>(
      app_state_manager,
      thread_pool,
      std::vector<std::shared_ptr<Listener>>{listener},
      server,
      processors,
      subscription_engine);
};

#endif  // KAGOME_TEST_CORE_API_TRANSPORT_LISTENER_TEST_HPP
