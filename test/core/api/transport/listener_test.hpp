/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "api/transport/listener.hpp"

#include <gtest/gtest.h>

#include "api/jrpc/jrpc_processor.hpp"
#include "api/jrpc/jrpc_server.hpp"
#include "api/service/impl/api_service_impl.hpp"
#include "application/impl/app_state_manager_impl.hpp"
#include "common/buffer.hpp"
#include "core/api/client/http_client.hpp"
#include "mock/core/api/transport/api_stub.hpp"
#include "mock/core/api/transport/jrpc_processor_stub.hpp"
#include "mock/core/application/app_configuration_mock.hpp"
#include "mock/core/application/app_state_manager_mock.hpp"
#include "mock/core/blockchain/block_tree_mock.hpp"
#include "mock/core/runtime/core_mock.hpp"
#include "mock/core/storage/trie/trie_storage_mock.hpp"
#include "primitives/event_types.hpp"
#include "runtime/runtime_context.hpp"
#include "subscription/extrinsic_event_key_repository.hpp"
#include "subscription/subscriber.hpp"
#include "testutil/asio_wait.hpp"
#include "testutil/outcome.hpp"
#include "testutil/prepare_loggers.hpp"
#include "transaction_pool/transaction_pool_error.hpp"

using namespace std::chrono_literals;
using namespace kagome::api;
using namespace kagome::common;
using namespace kagome::subscription;
using namespace kagome::primitives;
using kagome::ThreadPool;
using kagome::Watchdog;
using kagome::application::AppConfigurationMock;
using kagome::application::AppStateManager;
using kagome::blockchain::BlockTree;
using kagome::blockchain::BlockTreeMock;
using kagome::primitives::events::ChainSubscriptionEngine;
using kagome::primitives::events::ChainSubscriptionEnginePtr;
using kagome::primitives::events::ExtrinsicSubscriptionEngine;
using kagome::primitives::events::ExtrinsicSubscriptionEnginePtr;
using kagome::primitives::events::StorageSubscriptionEngine;
using kagome::primitives::events::StorageSubscriptionEnginePtr;
using kagome::runtime::CoreMock;
using kagome::storage::trie::TrieStorage;
using kagome::storage::trie::TrieStorageMock;
using kagome::subscription::ExtrinsicEventKeyRepository;
using testing::Return;
using testing::ReturnRef;

template <typename ListenerImpl,
          typename =
              std::enable_if_t<std::is_base_of_v<Listener, ListenerImpl>, void>>
struct ListenerTest : public ::testing::Test {
  static void SetUpTestCase() {
    testutil::prepareLoggers();
  }

  template <class T>
  using sptr = std::shared_ptr<T>;

 protected:
  using Endpoint = boost::asio::ip::tcp::endpoint;
  using Context = RpcContext;
  using Socket = boost::asio::ip::tcp::socket;
  using Timer = boost::asio::steady_timer;
  using Duration = boost::asio::steady_timer::duration;

  std::shared_ptr<RpcContext> rpc_context = std::make_shared<RpcContext>(1);
  std::shared_ptr<Context> main_context = std::make_shared<Context>(1);

  int64_t payload;
  std::string request;
  std::string response;

  void SetUp() override {
    payload = 0xABCDEF;

    request = R"({"jsonrpc":"2.0","method":"echo","id":0,"params":[)"
            + std::to_string(payload) + "]}";
    response =
        R"({"jsonrpc":"2.0","id":0,"result":)" + std::to_string(payload) + "}";

    auto seed = static_cast<unsigned int>(
        std::chrono::system_clock::now().time_since_epoch().count());
    auto rand = rand_r(&seed);

    endpoint.address(boost::asio::ip::address::from_string("127.0.0.1"));
    endpoint.port(1024 + rand % (65536 - 1024));  // random non-sudo port
    ON_CALL(app_config, rpcEndpoint()).WillByDefault(ReturnRef(endpoint));
    ON_CALL(app_config, maxWsConnections()).WillByDefault(Return(100));

    listener = std::make_shared<ListenerImpl>(
        *app_state_manager, main_context, app_config, session_config);

    service = std::make_shared<ApiServiceImpl>(
        *app_state_manager,
        std::vector<std::shared_ptr<Listener>>({listener}),
        server,
        std::vector<std::shared_ptr<JRpcProcessor>>(processors),
        storage_events_engine,
        chain_events_engine,
        ext_events_engine,
        ext_event_key_repo,
        block_tree,
        trie_storage,
        core,
        watchdog,
        rpc_context);
  }

  void TearDown() override {
    request.clear();
    response.clear();
    service.reset();
  }

  typename ListenerImpl::SessionImpl::Configuration session_config{};

  Endpoint endpoint;
  kagome::application::AppConfigurationMock app_config;

  sptr<kagome::application::AppStateManager> app_state_manager =
      std::make_shared<kagome::application::AppStateManagerImpl>();

  sptr<ApiStub> api = std::make_shared<ApiStub>();

  sptr<JRpcServer> server = std::make_shared<JRpcServerImpl>();

  std::vector<std::shared_ptr<JRpcProcessor>> processors{
      std::make_shared<JrpcProcessorStub>(server, api)};

  std::shared_ptr<Listener> listener;

  StorageSubscriptionEnginePtr storage_events_engine =
      std::make_shared<StorageSubscriptionEngine>();
  ChainSubscriptionEnginePtr chain_events_engine =
      std::make_shared<ChainSubscriptionEngine>();
  ExtrinsicSubscriptionEnginePtr ext_events_engine =
      std::make_shared<ExtrinsicSubscriptionEngine>();

  std::shared_ptr<ExtrinsicEventKeyRepository> ext_event_key_repo =
      std::make_shared<ExtrinsicEventKeyRepository>();

  std::shared_ptr<BlockTree> block_tree = std::make_shared<BlockTreeMock>();
  std::shared_ptr<TrieStorage> trie_storage =
      std::make_shared<TrieStorageMock>();
  std::shared_ptr<CoreMock> core = std::make_shared<CoreMock>();
  std::shared_ptr<Watchdog> watchdog = std::make_shared<Watchdog>();

  sptr<ApiService> service;
};
