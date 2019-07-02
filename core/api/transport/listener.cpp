/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/transport/listener.hpp"

namespace kagome::server {

  Listener::Listener(boost::asio::io_context &context,
                     boost::asio::ip::tcp::endpoint endpoint)
      : context_(context),
        acceptor_(context_, std::move(endpoint)),
        session_manager_(context_) {}

  void Listener::doAccept() {
    acceptor_.async_accept(
        [this](boost::system::error_code ec, Session::Socket socket) {
          if (!ec) {
            if (state_ != State::WORKING) {
              // TODO(yuraz): PRE-230 add log
              return;
            }
            auto id = session_manager_.newSession(std::move(socket));
            auto session = session_manager_[id];
            on_new_session_(session);
          }
        });
  }

}  // namespace kagome::server
