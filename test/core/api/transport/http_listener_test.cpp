/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 *
 * Kagome is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * Kagome is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
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
