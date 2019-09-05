/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/transport/beast/listener_impl.hpp"

namespace kagome::api {

  void BeastListener::start(Listener::NewSessionHandler on_new_session) {}

  void BeastListener::stop() {}

  void BeastListener::acceptOnce(
      std::function<void(std::shared_ptr<Session>)> on_new_session) {}

}  // namespace kagome::api
