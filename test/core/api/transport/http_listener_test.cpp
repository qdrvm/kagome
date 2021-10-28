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

  ASSERT_NO_THROW(listener->prepare());
  ASSERT_NO_THROW(service->prepare());

  ASSERT_NO_THROW(listener->start());
  ASSERT_NO_THROW(service->start());

  std::thread client_thread(
      [mc = main_context, cc = client_context](
          Endpoint endpoint, std::string request, std::string response) {
        auto client = std::make_shared<HttpClient>(*cc);
        ASSERT_TRUE(client->connect(endpoint));
        client->query(request,
                      [mc, response](outcome::result<std::string> res) {
                        ASSERT_TRUE(res);
                        ASSERT_EQ(res.value(), response);
                        mc->stop();
                      });
      },
      listener_config.endpoint,
      request,
      response);

  main_context->run_for(std::chrono::seconds(2));
  client_thread.join();

  ASSERT_NO_THROW(service->stop());
  ASSERT_NO_THROW(listener->stop());
}
