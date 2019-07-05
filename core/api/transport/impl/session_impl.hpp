/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_API_TRANSPORT_IMPL_SESSION_IMPL_HPP
#define KAGOME_CORE_API_TRANSPORT_IMPL_SESSION_IMPL_HPP

#include "api/transport/session.hpp"

namespace kagome::server {
  class SessionImpl : public Session {
   public:
    /**
     * @brief constructor
     * @param socket boost asio socket
     * @param id session id
     * @param context boost asio context
     * @param manager session manager
     */
    SessionImpl(Socket socket, Context &context);

    ~SessionImpl() override = default;

    /**
     * @brief starts listening on socket
     */
    void start() override;

    /**
     * @brief closes connection and session
     */
    void stop() override;

   private:
    /**
     * starts listening to requests
     */
    void asyncRead();

    /**
     * @brief runs when responce is obtained
     * @param response json string response
     */
    void processResponse(std::string response);

    /**
     * @brief sends response
     * @param response json result string
     */
    void asyncWrite(std::string response);

    /**
     * @brief is called when no requests for too long time
     * closes session
     */
    void processHeartBeat();

    Socket socket_;           ///< socket
    Timer heartbeat_;         ///< heartbeat timer
    Streambuf buffer_;        ///< request buffer
  };
}  // namespace kagome::server

#endif  // KAGOME_CORE_API_TRANSPORT_IMPL_SESSION_IMPL_HPP
