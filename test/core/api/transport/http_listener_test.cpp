/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "core/api/transport/listener_test.hpp"

#if defined(BACKWARD_HAS_BACKTRACE)
#include <backward.hpp>
#endif

#include "api/transport/impl/ws/ws_listener_impl.hpp"
#include "core/api/client/http_client.hpp"

using kagome::api::WsListenerImpl;
using test::HttpClient;

using HttpListenerTest = ListenerTest<WsListenerImpl>;

/**
 * @given running HTTP transport based RPC service
 * @when do simple request to RPC
 * @then response contains expected value
 */
TEST_F(HttpListenerTest, EchoSuccess) {
#if defined(BACKWARD_HAS_BACKTRACE)
  backward::SignalHandling sh;
#endif

  std::unique_ptr<std::thread> asio_runner;

  app_state_manager->atLaunch([&] {
    asio_runner =
        std::make_unique<std::thread>([ctx{main_context}, watchdog{watchdog}] {
          soralog::util::setThreadName("asio_runner");
          watchdog->run(ctx);
        });
    return true;
  });

  std::thread watchdog_thread([watchdog{watchdog}] {
    soralog::util::setThreadName("watchdog");
    watchdog->checkLoop(kagome::kWatchdogDefaultTimeout);
  });

  app_state_manager->atShutdown([watchdog{watchdog}] { watchdog->stop(); });

  std::unique_ptr<std::thread> client_thread;

  post(*main_context, [&] {
    client_thread = std::make_unique<std::thread>(
        [&](Endpoint endpoint, std::string request, std::string response) {
          soralog::util::setThreadName("client");

          auto local_context = std::make_shared<Context>();

          bool time_is_out;

          std::this_thread::sleep_for(1s);  // Gives chance app to be started
          post(*local_context, [&] {
            auto client = std::make_shared<HttpClient>(*local_context);

            ASSERT_OUTCOME_SUCCESS(client->connect(endpoint));

            client->query(request, [&](outcome::result<std::string> res) {
              ASSERT_OUTCOME_SUCCESS(res);
              EXPECT_EQ(res.value(), response);
              client->disconnect();
              time_is_out = false;
              local_context->stop();
            });
          });

          time_is_out = true;
          local_context->run_for(2s);
          EXPECT_FALSE(time_is_out);

          EXPECT_EQ(app_state_manager->state(), AppStateManager::State::Works);
          app_state_manager->shutdown();
        },
        endpoint,
        request,
        response);
  });

  app_state_manager->run();

  client_thread->join();
  asio_runner->join();
  watchdog_thread.join();
}
