/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_API_TRANSPORT_SESSION_MANAGER_HPP
#define KAGOME_CORE_API_TRANSPORT_SESSION_MANAGER_HPP

#include <memory>
#include <unordered_map>

#include <outcome/outcome.hpp>
#include "api/transport/session.hpp"

namespace kagome::server {

  class SessionManager {
    template <class T>
    using sptr = std::shared_ptr<T>;
    using Connection = boost::signals2::connection;
    using ClosedSignal = boost::signals2::signal<void(Session::Id)>;

   public:
    explicit SessionManager(boost::asio::io_context &context);

    inline Session::Id generateSessionId() {
      return ++lastSessionid_;
    }

    /**
     * @brief creates new session from socket
     * @param socket opened socket
     * @return id of newly created session
     */
    Session::Id newSession(Session::Socket socket);

    /**
     * @brief looks for session by index
     * @param id session id to look for
     * @return sptr to session or nullptr if not found
     */
    sptr<Session> operator[](Session::Id id);

    /**
     * subscribes for `session closed` signal
     * @param signal
     * @return connection
     */
    Connection subscribeOnClosed(ClosedSignal &signal);

   private:
    /**
     * @brief processes `session closed` event
     * @param id session id
     */
    void processSessionClosed(Session::Id id);

    Session::Id lastSessionid_{0u};     ///< last session id, 0 is reserved
    boost::asio::io_context &context_;  ///< reference to io_context object
    std::unordered_map<Session::Id, std::shared_ptr<Session>> sessions_;
  };

}  // namespace kagome::server

#endif  // KAGOME_CORE_API_TRANSPORT_SESSION_MANAGER_HPP
