/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "core/api/transport/listener_test.hpp"

#include "api/transport/impl/http/http_listener_impl.hpp"
#include "core/api/client/http_client.hpp"

using kagome::api::HttpListenerImpl;
using test::HttpClient;

using HttpListenerTest = ListenerTest<HttpListenerImpl>;

/**
 * @given runing HTTP transport based RPC service
 * @when do simple request to RPC
 * @then response contains expected value
 */

TEST_F(HttpListenerTest, EchoSuccess) {
  const Duration timeout_duration = std::chrono::milliseconds(200);

  std::shared_ptr<HttpClient> client;

  ASSERT_NO_THROW(service->start());

  client = std::make_shared<HttpClient>(*client_context);

  std::thread client_thread([this, client] {
    ASSERT_TRUE(client->connect(endpoint));
    client->query(request,
                  [&response = response](outcome::result<std::string> res) {
                    ASSERT_TRUE(res);
                    ASSERT_EQ(res.value(), response);
                  });
  });

  main_context->run_for(timeout_duration);
  client_thread.join();
}
