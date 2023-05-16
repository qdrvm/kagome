/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/transport/impl/ws/ws_session.hpp"

#include <thread>

#include <boost/asio/dispatch.hpp>
#include <boost/config.hpp>

namespace boost::beast {
  template <class NextLayer, class DynamicBuffer>
  void teardown(role_type role,
                buffered_read_stream<NextLayer, DynamicBuffer> &s,
                error_code &ec) {
    using boost::beast::websocket::teardown;
    teardown(role, s.next_layer(), ec);
  }

  template <class NextLayer, class DynamicBuffer, class TeardownHandler>
  void async_teardown(role_type role,
                      buffered_read_stream<NextLayer, DynamicBuffer> &s,
                      TeardownHandler &&handler) {
    using boost::beast::websocket::async_teardown;
    async_teardown(role, s.next_layer(), std::move(handler));
  }
}  // namespace boost::beast

namespace kagome::api {
  static constexpr boost::string_view kServerName = "Kagome";

  WsSessionImpl::WsSessionImpl(std::shared_ptr<WsSession> impl,
                               SessionId id,
                               SessionType type,
                               bool unsafe_allowed)
      : impl_{std::move(impl)},
        id_{id},
        type_{type},
        unsafe_allowed_{unsafe_allowed} {}

  void WsSessionImpl::respond(std::string_view response) {
    std::unique_lock lock{mutex_};
    if (auto impl = impl_) {
      lock.unlock();
      impl->respond(response);
    }
  }

  void WsSessionImpl::post(std::function<void()> cb) {
    std::unique_lock lock{mutex_};
    if (auto impl = impl_) {
      lock.unlock();
      impl->post(std::move(cb));
    }
  }

  void WsSessionImpl::close() {
    std::unique_lock lock{mutex_};
    impl_.reset();
    lock.unlock();
    notifyOnClose(id(), type());
  }

  WsSession::WsSession(Session::Context &context,
                       GetId get_id,
                       OnSession on_session,
                       AllowUnsafe allow_unsafe,
                       WsSession::Configuration config)
      : get_id_{std::move(get_id)},
        on_session_{std::move(on_session)},
        allow_unsafe_{allow_unsafe},
        config_{config},
        stream_{boost::asio::make_strand(context)} {}

  void WsSession::start() {
    boost::asio::dispatch(stream_.get_executor(),
                          boost::beast::bind_front_handler(&WsSession::httpRead,
                                                           shared_from_this()));
  }

  void WsSession::reject() {
    auto &res = http_response_.emplace();
    res.result(boost::beast::http::status::too_many_requests);
    res.body() = "Too many requests.\n";
    res.set(boost::beast::http::field::server, kServerName);
    res.set(boost::beast::http::field::content_type, "text/plain");
    res.keep_alive(false);
    httpWrite();
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
      sessionClose();
      if (on_ws_close_) {
        on_ws_close_();
      }
    }
  }

  void WsSession::connectOnWsSessionCloseHandler(
      WsSession::OnWsSessionCloseHandler &&handler) {
    on_ws_close_ = std::move(handler);
  }

  void WsSession::handleRequest(std::string_view data) {
    std::shared_ptr<WsSessionImpl> session;
    if (is_ws_) {
      session = session_.lock();
      if (not session) {
        stop();
        return;
      }
    } else {
      session = sessionMake();
    }
    session->processRequest(data, session);
  }

  void WsSession::asyncRead() {
    stream_.async_read(rbuffer_,
                       boost::beast::bind_front_handler(&WsSession::onRead,
                                                        shared_from_this()));
  }

  void WsSession::respond(std::string_view response) {
    post([self{shared_from_this()}, response{std::string{response}}] {
      if (not self->is_ws_) {
        self->sessionClose();
        if (self->http_response_) {
          SL_WARN(self->logger_,
                  "bug, WsSession::respond called more than once on http");
          return;
        }
        auto &req = self->http_request_->get();
        auto &res = self->http_response_.emplace(
            boost::beast::http::status::ok, req.version(), std::move(response));
        res.set(boost::beast::http::field::server, kServerName);
        res.set(boost::beast::http::field::content_type, "application/json");
        res.keep_alive(req.keep_alive());
        self->httpWrite();
        return;
      }
      self->pending_responses_.emplace(std::move(response));
      self->asyncWrite();
    });
  }

  void WsSession::asyncWrite() {
    if (writing_in_progress_) {
      return;
    }
    if (pending_responses_.empty()) {
      return;
    }
    writing_in_progress_ = true;
    stream_.text(true);
    stream_.async_write(boost::asio::buffer(pending_responses_.front()),
                        boost::beast::bind_front_handler(&WsSession::onWrite,
                                                         shared_from_this()));
  }

  void WsSession::wsAccept() {
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
    stream_.async_accept(http_request_->get(),
                         boost::beast::bind_front_handler(&WsSession::onAccept,
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
    is_ws_ = true;
    sessionMake();

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
    writing_in_progress_ = false;
    assert(bytes_transferred == pending_responses_.front().size());
    pending_responses_.pop();
    asyncWrite();
  }

  void WsSession::reportError(boost::system::error_code ec,
                              std::string_view message) {
    SL_ERROR(logger_, "error occurred: {}, message: {}", message, ec);
  }

  void WsSession::post(std::function<void()> cb) {
    boost::asio::post(stream_.get_executor(), std::move(cb));
  }

  bool WsSession::isUnsafeAllowed() const {
    return allow_unsafe_.allow(
        stream_.next_layer().next_layer().socket().remote_endpoint());
  }

  std::shared_ptr<WsSessionImpl> WsSession::sessionMake() {
    auto session = std::make_shared<WsSessionImpl>(
        shared_from_this(),
        get_id_(),
        is_ws_ ? SessionType::kWs : SessionType::kHttp,
        isUnsafeAllowed());
    if (on_session_) {
      (*on_session_)(session);
    }
    session_ = session;
    return session;
  }

  void WsSession::sessionClose() {
    if (auto session = session_.lock()) {
      session->close();
    }
    session_.reset();
  }

  void WsSession::httpClose() {
    bool already_stopped = false;
    if (stopped_.compare_exchange_strong(already_stopped, true)) {
      stream_.next_layer().next_layer().close();
      if (on_ws_close_) {
        on_ws_close_();
      }
    }
  }

  void WsSession::httpRead() {
    http_request_.emplace();
    http_request_->body_limit(config_.max_request_size);
    stream_.next_layer().next_layer().expires_after(config_.operation_timeout);
    boost::beast::http::async_read(
        stream_.next_layer().next_layer(),
        stream_.next_layer().buffer(),
        http_request_->get(),
        [self{shared_from_this()}](boost::system::error_code ec, size_t) {
          if (ec) {
            self->httpClose();
            return;
          }
          auto &req = self->http_request_->get();
          if (boost::beast::websocket::is_upgrade(req)) {
            self->wsAccept();
            return;
          }
          // allow only POST method
          if (req.method() != boost::beast::http::verb::post) {
            auto &res = self->http_response_.emplace(
                boost::beast::http::status::bad_request,
                req.version(),
                "Unsupported HTTP-method");
            res.set(boost::beast::http::field::server, kServerName);
            res.set(boost::beast::http::field::content_type, "text/plain");
            res.keep_alive(req.keep_alive());
            self->httpWrite();
            return;
          }
          self->http_response_.reset();
          self->handleRequest(req.body());
        });
  }

  void WsSession::httpWrite() {
    http_response_->prepare_payload();
    boost::beast::http::async_write(
        stream_.next_layer(),
        *http_response_,
        [self = shared_from_this()](boost::system::error_code ec, size_t) {
          if (ec) {
            self->httpClose();
            return;
          }
          if (self->http_response_->need_eof()) {
            self->httpClose();
            return;
          }
          self->httpRead();
        });
  }
}  // namespace kagome::api
