/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "core/api/transport/listener_test.hpp"

#include "api/transport/impl/ws/ws_listener_impl.hpp"
#include "core/api/client/ws_client.hpp"

using kagome::api::WsListenerImpl;
using test::WsClient;

using WsListenerTest = ListenerTest<WsListenerImpl>;

TEST_F(WsListenerTest, EchoSuccess) {
  const Duration timeout_duration = std::chrono::milliseconds(200);

  std::shared_ptr<WsClient> client;

  ASSERT_NO_THROW(service->start());

  client = std::make_shared<WsClient>(*client_context);

  std::thread client_thread([this, client] {
    ASSERT_TRUE(client->connect(endpoint));
    client->query(request,
                  [&response = response](outcome::result<std::string> res) {
                    ASSERT_TRUE(res);
                    ASSERT_EQ(res.value(), response);
                  });
    client->disconnect();
  });

  main_context->run_for(timeout_duration);
  client_thread.join();
}
