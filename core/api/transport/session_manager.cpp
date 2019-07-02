/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/transport/session_manager.hpp"

namespace kagome::server {

  SessionManager::SessionManager(boost::asio::io_context &context)
      : context_(context) {}

  Session::Id SessionManager::newSession(Session::Socket socket) {
    auto id = generateSessionId();
    sessions_.insert(
        {id, std::make_shared<Session>(std::move(socket), id, context_, *this)});
    return id;
  }

  SessionManager::sptr<kagome::server::Session> SessionManager::operator[](
      Session::Id id) {
    auto it = sessions_.find(id);
    if (std::end(sessions_) == it) {
      return nullptr;
    }
    return it->second;
  }

  SessionManager::Connection SessionManager::subscribeOnClosed(
      SessionManager::ClosedSignal &signal) {
    return signal.connect([this](Session::Id id) { processSessionClosed(id); });
  }

  void SessionManager::processSessionClosed(Session::Id id) {
    sessions_.erase(id);
  }

}  // namespace kagome::server
