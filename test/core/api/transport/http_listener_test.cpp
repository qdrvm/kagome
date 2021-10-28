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
  backward::SignalHandling sh;

  ASSERT_TRUE(listener->prepare());
  ASSERT_TRUE(service->prepare());

  ASSERT_TRUE(listener->start());
  ASSERT_TRUE(service->start());

  std::thread client_thread(
      [&](Endpoint endpoint, std::string request, std::string response) {
        auto local_context = std::make_shared<Context>();

        auto client = std::make_shared<HttpClient>(*local_context);

        local_context->post([&] {
          ASSERT_OUTCOME_SUCCESS_TRY(client->connect(endpoint));

          client->query(request, [&](outcome::result<std::string> res) {
            ASSERT_OUTCOME_SUCCESS_TRY(res);
            EXPECT_EQ(res.value(), response);
            local_context->stop();
          });
        });

        local_context->run_for(2s);
        main_context->stop();
      },
      listener_config.endpoint,
      request,
      response);

  main_context->run_for(2s);
  client_thread.join();

  ASSERT_NO_THROW(service->stop());
  ASSERT_NO_THROW(listener->stop());
}
