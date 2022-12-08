/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/transport/impl/ws/ws_session.hpp"

#include <thread>

#include <boost/asio/dispatch.hpp>
#include <boost/config.hpp>

namespace kagome::api {

  WsSession::WsSession(Context &context, Configuration config, SessionId id)
      : strand_(boost::asio::make_strand(context)),
        socket_(strand_),
        config_{config},
        stream_(socket_),
        id_(id) {}

  void WsSession::start() {
    boost::asio::dispatch(stream_.get_executor(),
                          boost::beast::bind_front_handler(&WsSession::onRun,
                                                           shared_from_this()));
    SL_DEBUG(logger_, "Session#{} BEGIN", id_);
  }

  void WsSession::reject() {
    stop(boost::beast::websocket::close_code::try_again_later);
    SL_DEBUG(logger_, "Session#{} END", id_);
  }

  void WsSession::stop() {
    // none is the default value for close_reason
    stop(boost::beast::websocket::close_code::none);
  }

  void WsSession::stop(boost::beast::websocket::close_code code) {
    bool already_stopped = false;
    if (stopped_.compare_exchange_strong(already_stopped, true)) {
      boost::system::error_code ec;
      stream_.close(boost::beast::websocket::close_reason(code), ec);
      boost::ignore_unused(ec);
      notifyOnClose(id_, type());
      if (on_ws_close_) {
        on_ws_close_();
      }
      SL_TRACE(logger_, "Session id = {} terminated, reason = {} ", id_, code);
    } else {
      SL_TRACE(logger_,
               "Session id = {} was already terminated. Doing nothing. Called "
               "for reason = {}",
               id_,
               code);
    }
  }

  void WsSession::connectOnWsSessionCloseHandler(
      WsSession::OnWsSessionCloseHandler &&handler) {
    on_ws_close_ = std::move(handler);
  }

  void WsSession::handleRequest(std::string_view data) {
    SL_DEBUG(logger_, "Session#{} IN:  {}", id_, data);
    processRequest(data, shared_from_this());
  }

  void WsSession::asyncRead() {
    stream_.async_read(rbuffer_,
                       boost::beast::bind_front_handler(&WsSession::onRead,
                                                        shared_from_this()));
  }

  kagome::api::Session::SessionId WsSession::id() const {
    return id_;
  }

  void WsSession::respond(std::string_view response) {
    SL_DEBUG(logger_, "Session#{} OUT: {}", id_, response);
    {
      std::lock_guard lg{cs_};
      pending_responses_.emplace(response);
    }
    asyncWrite();
  }

  void WsSession::asyncWrite() {
    bool val = false;
    if (writing_in_progress_.compare_exchange_strong(val, true)) {
      std::lock_guard lg{cs_};
      if (wbuffer_.size() == 0 and not pending_responses_.empty()) {
        boost::asio::buffer_copy(
            wbuffer_.prepare(pending_responses_.front().size()),
            boost::asio::const_buffer(pending_responses_.front().data(),
                                      pending_responses_.front().size()));
        wbuffer_.commit(pending_responses_.front().size());
        stream_.text(true);
        pending_responses_.pop();

        stream_.async_write(wbuffer_.data(),
                            boost::beast::bind_front_handler(
                                &WsSession::onWrite, shared_from_this()));
      }
    }
  }

  void WsSession::onRun() {
    // Set suggested timeout settings for the websocket
    stream_.set_option(boost::beast::websocket::stream_base::timeout::suggested(
        boost::beast::role_type::server));

    // Set a decorator to change the Server of the handshake
    stream_.set_option(boost::beast::websocket::stream_base::decorator(
        [](boost::beast::websocket::response_type &res) {
          res.set(boost::beast::http::field::server,
                  std::string(BOOST_BEAST_VERSION_STRING)
                      + " websocket-server-async");
        }));
    // Accept the websocket handshake
    stream_.async_accept(boost::beast::bind_front_handler(&WsSession::onAccept,
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

    asyncRead();
  }

  void WsSession::onWrite(boost::system::error_code ec,
                          std::size_t bytes_transferred) {
    if (ec) {
      reportError(ec, "failed to write message");
      return stop();
    }

    wbuffer_.consume(bytes_transferred);

    if (wbuffer_.size() > 0) {
      stream_.async_write(wbuffer_.data(),
                          boost::beast::bind_front_handler(&WsSession::onWrite,
                                                           shared_from_this()));
    } else {
      std::lock_guard lg{cs_};
      if (not pending_responses_.empty()) {
        boost::asio::buffer_copy(
            wbuffer_.prepare(pending_responses_.front().size()),
            boost::asio::const_buffer(pending_responses_.front().data(),
                                      pending_responses_.front().size()));
        wbuffer_.commit(pending_responses_.front().size());
        stream_.text(true);
        pending_responses_.pop();

        stream_.async_write(wbuffer_.data(),
                            boost::beast::bind_front_handler(
                                &WsSession::onWrite, shared_from_this()));
      } else {
        writing_in_progress_ = false;
      }
    }
  }

  void WsSession::reportError(boost::system::error_code ec,
                              std::string_view message) {
    SL_ERROR(logger_, "error occurred: {}, message: {}", message, ec);
  }

}  // namespace kagome::api
