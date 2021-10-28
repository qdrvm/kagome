/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "core/api/transport/listener_test.hpp"

#include <backward.hpp>

#include "api/transport/impl/http/http_listener_impl.hpp"
#include "core/api/client/http_client.hpp"

using kagome::api::HttpListenerImpl;
using test::HttpClient;

using HttpListenerTest = ListenerTest<HttpListenerImpl>;

/**
 * @given running HTTP transport based RPC service
 * @when do simple request to RPC
 * @then response contains expected value
 */
TEST_F(HttpListenerTest, EchoSuccess) {
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
            auto client = std::make_shared<HttpClient>(*local_context);

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
        listener_config.endpoint,
        request,
        response)
        .detach();
  });

  app_state_manager->run();
}
