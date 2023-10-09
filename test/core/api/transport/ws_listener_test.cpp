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
#include "core/api/client/ws_client.hpp"

using kagome::api::WsListenerImpl;
using test::WsClient;

using WsListenerTest = ListenerTest<WsListenerImpl>;

/**
 * @given runing websocket transport based RPC service
 * @when do simple request to RPC
 * @then response contains expected value
 */
TEST_F(WsListenerTest, EchoSuccess) {
#if defined(BACKWARD_HAS_BACKTRACE)
  backward::SignalHandling sh;
#endif

  app_state_manager->atLaunch([ctx{main_context}] {
    std::thread([ctx] { ctx->run_for(3s); }).detach();
    return true;
  });

  main_context->post([&] {
    std::thread(
        [&](Endpoint endpoint, std::string request, std::string response) {
          auto local_context = std::make_shared<Context>();

          bool time_is_out;

          local_context->post([&] {
            auto client = std::make_shared<WsClient>(*local_context);

            ASSERT_OUTCOME_SUCCESS_TRY(client->connect(endpoint));

            client->query(request, [&](outcome::result<std::string> res) {
              ASSERT_OUTCOME_SUCCESS_TRY(res);
              EXPECT_EQ(res.value(), response);
              client->disconnect();
              time_is_out = false;
              local_context->stop();
            });
          });

          time_is_out = true;
          local_context->run_for(2s);
          EXPECT_FALSE(time_is_out);

          EXPECT_TRUE(app_state_manager->state()
                      == AppStateManager::State::Works);
          app_state_manager->shutdown();
        },
        endpoint,
        request,
        response)
        .detach();
  });

  app_state_manager->run();
}
