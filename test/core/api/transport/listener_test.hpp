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
#include "api/service/impl/api_service_impl.hpp"
#include "api/transport/rpc_thread_pool.hpp"
#include "application/impl/app_state_manager_impl.hpp"
#include "common/buffer.hpp"
#include "core/api/client/http_client.hpp"
#include "mock/core/api/transport/api_stub.hpp"
#include "mock/core/api/transport/jrpc_processor_stub.hpp"
#include "mock/core/application/app_state_manager_mock.hpp"
#include "mock/core/blockchain/block_tree_mock.hpp"
#include "mock/core/storage/trie/trie_storage_mock.hpp"
#include "primitives/event_types.hpp"
#include "subscription/extrinsic_event_key_repository.hpp"
#include "subscription/subscriber.hpp"
#include "testutil/outcome.hpp"
#include "testutil/prepare_loggers.hpp"
#include "transaction_pool/transaction_pool_error.hpp"

using namespace std::chrono_literals;
using namespace kagome::api;
using namespace kagome::common;
using namespace kagome::subscription;
using namespace kagome::primitives;
using kagome::application::AppStateManager;
using kagome::blockchain::BlockTree;
using kagome::blockchain::BlockTreeMock;
using kagome::primitives::events::ChainSubscriptionEngine;
using kagome::primitives::events::ChainSubscriptionEnginePtr;
using kagome::primitives::events::ExtrinsicSubscriptionEngine;
using kagome::primitives::events::ExtrinsicSubscriptionEnginePtr;
using kagome::primitives::events::StorageSubscriptionEngine;
using kagome::primitives::events::StorageSubscriptionEnginePtr;
using kagome::storage::trie::TrieStorage;
using kagome::storage::trie::TrieStorageMock;
using kagome::subscription::ExtrinsicEventKeyRepository;

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
  }

  void TearDown() override {
    request.clear();
    response.clear();

    service.reset();
  }

  typename ListenerImpl::Configuration listener_config = [] {
    typename ListenerImpl::Configuration config{};
    config.endpoint.address(boost::asio::ip::address::from_string("127.0.0.1"));
    config.endpoint.port(4321);
    config.ws_max_connections = 100;
    return config;
  }();

  typename ListenerImpl::SessionImpl::Configuration session_config{};

  sptr<kagome::application::AppStateManager> app_state_manager =
      std::make_shared<kagome::application::AppStateManagerImpl>();

  kagome::api::RpcThreadPool::Configuration config = {1, 1};

  sptr<kagome::api::RpcThreadPool> thread_pool =
      std::make_shared<kagome::api::RpcThreadPool>(rpc_context, config);

  sptr<ApiStub> api = std::make_shared<ApiStub>();

  sptr<JRpcServer> server = std::make_shared<JRpcServerImpl>();

  std::vector<std::shared_ptr<JRpcProcessor>> processors{
      std::make_shared<JrpcProcessorStub>(server, api)};

  std::shared_ptr<Listener> listener = std::make_shared<ListenerImpl>(
      app_state_manager, main_context, listener_config, session_config);

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

  sptr<ApiService> service = std::make_shared<ApiServiceImpl>(
      app_state_manager,
      thread_pool,
      ApiServiceImpl::ListenerList{{listener}},
      server,
      ApiServiceImpl::ProcessorSpan{processors},
      storage_events_engine,
      chain_events_engine,
      ext_events_engine,
      ext_event_key_repo,
      block_tree,
      trie_storage);
};

#endif  // KAGOME_TEST_CORE_API_TRANSPORT_LISTENER_TEST_HPP
