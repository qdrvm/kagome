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

#include "api/transport/impl/ws/ws_session.hpp"

#include <cstring>

#include <boost/asio/dispatch.hpp>
#include <boost/config.hpp>
#include "outcome/outcome.hpp"

namespace kagome::api {

  WsSession::WsSession(Socket socket, Configuration config)
      : config_{config}, ws_(std::move(socket)) {}

  void WsSession::start() {
    boost::asio::dispatch(ws_.get_executor(),
                          boost::beast::bind_front_handler(&WsSession::onRun,
                                                           shared_from_this()));
  }

  void WsSession::stop() {
    boost::system::error_code ec;
    ws_.close(boost::beast::websocket::close_reason(), ec);
    boost::ignore_unused(ec);
  }

  void WsSession::handleRequest(std::string_view data) {
    processRequest(data, shared_from_this());
  }

  void WsSession::asyncRead() {
    ws_.async_read(rbuffer_,
                   boost::beast::bind_front_handler(&WsSession::onRead,
                                                    shared_from_this()));
  }

  void WsSession::asyncWrite() {
    ws_.async_write(wbuffer_.data(),
                    boost::beast::bind_front_handler(&WsSession::onWrite,
                                                     shared_from_this()));
  }

  void WsSession::respond(std::string_view response) {
    auto x = wbuffer_.prepare(response.size());
    std::copy(response.data(),
              response.data() + response.size(),
              reinterpret_cast<char *>(x.data()));  // NOLINT
    wbuffer_.commit(response.size());
    ws_.text(true);
    asyncWrite();
  }

  void WsSession::onRun() {
    // Set suggested timeout settings for the websocket
    ws_.set_option(boost::beast::websocket::stream_base::timeout::suggested(
        boost::beast::role_type::server));

    // Set a decorator to change the Server of the handshake
    ws_.set_option(boost::beast::websocket::stream_base::decorator(
        [](boost::beast::websocket::response_type &res) {
          res.set(boost::beast::http::field::server,
                  std::string(BOOST_BEAST_VERSION_STRING)
                      + " websocket-server-async");
        }));
    // Accept the websocket handshake
    ws_.async_accept(boost::beast::bind_front_handler(&WsSession::onAccept,
                                                      shared_from_this()));
  }

  void WsSession::onAccept(boost::system::error_code ec) {
    if (ec) {
      auto error_message = (ec == WsError::closed) ? "connection was closed"
                                                   : "unknown error occurred";
      reportError(ec, error_message);
      stop();
      return;
    }

	  asyncRead();
  };

  void WsSession::onRead(boost::system::error_code ec,
                         std::size_t bytes_transferred) {
    if (ec) {
      auto error_message = (ec == WsError::closed) ? "connection was closed"
                                                   : "unknown error occurred";
      reportError(ec, error_message);
      stop();
      return;
    }

    handleRequest(
        {static_cast<char *>(rbuffer_.data().data()), bytes_transferred});

    rbuffer_.consume(bytes_transferred);
  }

  void WsSession::onWrite(boost::system::error_code ec,
                          std::size_t bytes_transferred) {
    if (ec) {
      reportError(ec, "failed to write message");
      return stop();
    }

    wbuffer_.consume(bytes_transferred);

	  asyncRead();
  }

  void WsSession::reportError(boost::system::error_code ec,
                              std::string_view message) {
    logger_->error("error occured: {}, code: {}", message, ec);
  }

}  // namespace kagome::api
