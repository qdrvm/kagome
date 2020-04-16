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

#ifndef KAGOME_CORE_API_TRANSPORT_LISTENER_HPP
#define KAGOME_CORE_API_TRANSPORT_LISTENER_HPP

#include <memory>

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <outcome/outcome.hpp>
#include "api/transport/session.hpp"

namespace kagome::api {
  /**
   * @brief server which listens for incoming connection,
   * accepts connections making session from socket
   */
  class Listener {
   protected:
    template <class T>
    using sptr = std::shared_ptr<T>;

    template <class T>
    using Signal = boost::signals2::signal<T>;
    using OnError = Signal<void(const outcome::result<void> &)>;

    using NewSessionHandler = std::function<void(const sptr<Session> &)>;

   public:
    virtual ~Listener() = default;

    /**
     * @brief starts listening
     */
    virtual void start(NewSessionHandler on_new_session) = 0;

    /**
     * @brief stops listening
     */
    virtual void stop() = 0;

   protected:
    /**
     * @brief accepts incoming connection
     */
    virtual void acceptOnce(NewSessionHandler on_new_session) = 0;
  };
}  // namespace kagome::api

#endif  // KAGOME_CORE_API_TRANSPORT_LISTENER_IMPL_HPP
