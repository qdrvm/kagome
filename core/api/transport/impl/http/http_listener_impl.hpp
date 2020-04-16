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

#ifndef KAGOME_CORE_API_TRANSPORT_IMPL_LISTENER_IMPL_HPP
#define KAGOME_CORE_API_TRANSPORT_IMPL_LISTENER_IMPL_HPP

#include "api/transport/listener.hpp"

#include "api/transport/impl/http/http_session.hpp"
#include "common/logger.hpp"

namespace kagome::api {
  /**
   * @brief server which listens for incoming connection,
   * accepts connections making session from socket
   */
  class HttpListenerImpl
      : public Listener,
        public std::enable_shared_from_this<HttpListenerImpl> {
    enum class State { READY, WORKING, STOPPED = READY };
    using Context = boost::asio::io_context;
    using Acceptor = boost::asio::ip::tcp::acceptor;
    using Endpoint = boost::asio::ip::tcp::endpoint;
    using Duration = boost::asio::steady_timer::duration;
    using Logger = common::Logger;

   public:
    using SessionImpl = HttpSession;

    /***
     * Listener configuration
     */
    struct Configuration {
      Endpoint endpoint{};  ///< listener endpoint
    };

    /**
     * @param context reference to boost::asio::io_context instance
     * @param endpoint loopback ip address to listen
     * @param http_config http session configuration
     */
    HttpListenerImpl(Context &context,
					 const Configuration &configuration,
					 SessionImpl::Configuration session_config);

    ~HttpListenerImpl() override = default;

    /**
     * @brief starts listener
     * @param on_new_session new session creation callback
     */
    void start(NewSessionHandler on_new_session) override;

    /**
     * @brief stops listener
     */
    void stop() override;

   private:
    /**
     * @brief accepts incoming connection
     */
    void acceptOnce(NewSessionHandler on_new_session) override;

    Context &context_;                        ///< io context
    Acceptor acceptor_;                       ///< connections acceptor
    State state_{State::READY};               ///< working state
	SessionImpl::Configuration session_config_;  /// http session configuration
    Logger logger_ = common::createLogger("api listener");  ///< logger instance
  };
}  // namespace kagome::api

#endif  // KAGOME_CORE_API_TRANSPORT_IMPL_LISTENER_IMPL_HPP
