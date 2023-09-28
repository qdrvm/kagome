/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#define BOOST_ASIO_DISABLE_CONCEPTS

#include "telemetry/impl/connection_impl.hpp"

#include <openssl/tls1.h>

namespace kagome::telemetry {

  std::size_t TelemetryConnectionImpl::instance_ = 0;

  TelemetryConnectionImpl::TelemetryConnectionImpl(
      std::shared_ptr<boost::asio::io_context> io_context,
      const TelemetryEndpoint &endpoint,
      OnConnectedCallback callback,
      std::shared_ptr<MessagePool> message_pool,
      std::shared_ptr<libp2p::basic::Scheduler> scheduler)
      : io_context_{std::move(io_context)},
        endpoint_{endpoint},
        callback_{std::move(callback)},
        message_pool_{std::move(message_pool)},
        scheduler_{std::move(scheduler)},
        ssl_ctx_{boost::asio::ssl::context::sslv23},
        resolver_{boost::asio::make_strand(*io_context_)} {
    BOOST_ASSERT(io_context_);
    BOOST_ASSERT(message_pool_);
    BOOST_ASSERT(scheduler_);
    auto instance_number = std::to_string(++instance_);
    queue_.set_capacity(message_pool_->capacity());
    log_ = log::createLogger("TelemetryConnection#" + instance_number,
                             "telemetry");
  }

  void TelemetryConnectionImpl::connect() {
    shutdown_requested_ = false;
    is_connected_ = false;

    // immediate return in case of empty host value
    if (endpoint_.uri().Host.empty()) {
      SL_ERROR(log_,
               "Host cannot be empty for telemetry endpoint {}",
               endpoint_.uri().to_string());
      return;
    }

    // setup defaults basing on URI schema
    auto &schema = endpoint_.uri().Schema;
    if (schema == "ws") {
      secure_ = false;
      port_ = 80;
    } else if (schema == "wss") {
      secure_ = true;
      port_ = 443;
    } else {
      SL_ERROR(log_,
               "Unsupported schema '{}' passed for telemetry endpoint {}",
               schema,
               endpoint_.uri().to_string());
      return;
    }

    // parse custom defined port value if any
    auto &port = endpoint_.uri().Port;
    if (not port.empty()) {
      try {
        auto port_int = std::stoi(port);
        if (port_int < 0 or port_int > 65536) {
          throw std::out_of_range("");
        }
        port_ = static_cast<uint16_t>(port_int);
      } catch (...) {
        // only std::invalid_argument or std::out_of_range are possible
        SL_ERROR(log_,
                 "Specified port value is not valid for endpoint {}",
                 endpoint_.uri().to_string());
        return;
      }
    }

    auto &path = endpoint_.uri().Path;
    path_ = path.empty() ? "/" : path;

    if (secure_) {
      ws_ = std::make_unique<WsSslStream>(
          boost::asio::make_strand(*io_context_), ssl_ctx_);
    } else {
      ws_ =
          std::make_unique<WsTcpStream>(boost::asio::make_strand(*io_context_));
    }

    SL_DEBUG(log_, "Connecting to endpoint {}", endpoint_.uri().to_string());
    resolver_.async_resolve(
        endpoint_.uri().Host,
        std::to_string(port_),
        boost::beast::bind_front_handler(&TelemetryConnectionImpl::onResolve,
                                         shared_from_this()));
  }

  const TelemetryEndpoint &TelemetryConnectionImpl::endpoint() const {
    return endpoint_;
  }

  void TelemetryConnectionImpl::send(const std::string &data) {
    if (not is_connected_) {
      return;
    }
    auto push = message_pool_->push(data, 1);
    if (not push) {
      return;
    }
    send(*push);
  }

  void TelemetryConnectionImpl::send(MessageHandle message_handle) {
    if (not is_connected_) {
      message_pool_->release(message_handle);
      return;
    }
    if (busy_) {
      if (queue_.full()) {
        message_pool_->release(message_handle);
        return;
      }
      queue_.push_back(message_handle);
    } else {
      busy_ = true;
      if (secure_) {
        write(*boost::relaxed_get<WsSslStreamPtr>(ws_), message_handle);
      } else {
        write(*boost::relaxed_get<WsTcpStreamPtr>(ws_), message_handle);
      }
    }
  }

  bool TelemetryConnectionImpl::isConnected() const {
    return is_connected_;
  }

  void TelemetryConnectionImpl::shutdown() {
    shutdown_requested_ = true;
    if (is_connected_) {
      close();
    }
  }

  boost::beast::lowest_layer_type<TelemetryConnectionImpl::SslStream>
      &TelemetryConnectionImpl::stream_lowest_layer() {
    return secure_ ? boost::beast::get_lowest_layer(
               *boost::relaxed_get<WsSslStreamPtr>(ws_))
                   : boost::beast::get_lowest_layer(
                       *boost::relaxed_get<WsTcpStreamPtr>(ws_));
  }

  template <typename WsStreamT>
  void TelemetryConnectionImpl::write(WsStreamT &ws,
                                      MessageHandle message_handle) {
    auto write_handler = [self{shared_from_this()}, message_handle, &ws](
                             boost::beast::error_code ec,
                             std::size_t bytes_transferred) {
      boost::ignore_unused(bytes_transferred);
      self->message_pool_->release(message_handle);
      if (ec) {
        self->is_connected_ = false;
        self->busy_ = false;
        self->releaseQueue();
        SL_ERROR(self->log_, "Unable to send data through websocket: {}", ec);
        self->reconnect();
        return;
      }
      if (self->queue_.empty()) {
        self->busy_ = false;
      } else {
        auto next = self->queue_.front();
        self->queue_.pop_front();
        self->write(ws, next);
      }
    };

    ws.async_write((*message_pool_)[message_handle], write_handler);
  }

  void TelemetryConnectionImpl::onResolve(
      boost::beast::error_code ec,
      boost::asio::ip::tcp::resolver::results_type results) {
    if (ec) {
      SL_ERROR(log_, "Unable to resolve host: {}", ec);
      reconnect();
      return;
    }

    stream_lowest_layer().expires_after(kConnectionTimeout);
    stream_lowest_layer().async_connect(
        results,
        boost::beast::bind_front_handler(&TelemetryConnectionImpl::onConnect,
                                         shared_from_this()));
  }

  void TelemetryConnectionImpl::onConnect(
      boost::beast::error_code ec,
      boost::asio::ip::tcp::resolver::results_type::endpoint_type endpoint) {
    if (ec) {
      SL_ERROR(log_, "Unable to connect to endpoint: {}", ec);
      reconnect();
      return;
    }

    if (secure_) {
      auto &ws = boost::relaxed_get<WsSslStreamPtr>(ws_);
      if (not SSL_set_tlsext_host_name(ws->next_layer().native_handle(),
                                       endpoint_.uri().Host.c_str())) {
        ec = boost::beast::error_code(static_cast<int>(::ERR_get_error()),
                                      boost::asio::error::get_ssl_category());
        reconnect();
        SL_ERROR(log_, "Unable to set SNI hostname: {}", ec);
      }
    }

    ws_handshake_hostname_ =
        endpoint_.uri().Host + ":" + std::to_string(endpoint.port());

    if (secure_) {
      auto &ws = *boost::relaxed_get<WsSslStreamPtr>(ws_);
      ws.next_layer().async_handshake(
          boost::asio::ssl::stream_base::client,
          boost::beast::bind_front_handler(
              &TelemetryConnectionImpl::onSslHandshake, shared_from_this()));
    } else {
      setOptionsAndRunWsHandshake(*boost::relaxed_get<WsTcpStreamPtr>(ws_));
    }
  }

  template <typename WsStreamT>
  void TelemetryConnectionImpl::setOptionsAndRunWsHandshake(WsStreamT &ws) {
    stream_lowest_layer().expires_never();

    ws.set_option(boost::beast::websocket::stream_base::timeout::suggested(
        boost::beast::role_type::client));
    // Set a decorator to change the User-Agent of the handshake
    ws.set_option(boost::beast::websocket::stream_base::decorator(
        [](boost::beast::websocket::request_type &req) {
          req.set(boost::beast::http::field::user_agent,
                  std::string(BOOST_BEAST_VERSION_STRING)
                      + " Kagome Telemetry Reporter");
        }));

    ws.async_handshake(
        ws_handshake_hostname_,
        path_,
        boost::beast::bind_front_handler(&TelemetryConnectionImpl::onHandshake,
                                         shared_from_this()));
  }

  void TelemetryConnectionImpl::onSslHandshake(boost::beast::error_code ec) {
    if (ec) {
      SL_ERROR(log_, "Unable to perform SSL handshake: {}", ec);
      reconnect();
      return;
    }
    BOOST_VERIFY(secure_);
    setOptionsAndRunWsHandshake(*boost::relaxed_get<WsSslStreamPtr>(ws_));
  }

  void TelemetryConnectionImpl::onHandshake(boost::beast::error_code ec) {
    if (ec) {
      SL_ERROR(log_, "Websocket handshake failed: {}", ec);
      reconnect();
      return;
    }
    if (shutdown_requested_) {
      close();
      return;
    }
    is_connected_ = true;
    SL_INFO(log_, "Connection established");
    reconnect_timeout_ = kInitialReconnectTimeout;
    callback_(shared_from_this());
  }

  void TelemetryConnectionImpl::releaseQueue() {
    for (auto handle : queue_) {
      message_pool_->release(handle);
    }
    queue_.clear();
  }

  void TelemetryConnectionImpl::close() {
    is_connected_ = false;
    if (secure_) {
      auto &ws = *boost::relaxed_get<WsSslStreamPtr>(ws_);
      ws.async_close(boost::beast::websocket::close_code::normal, [](boost::beast::error_code) {});
    } else {
      auto &ws = *boost::relaxed_get<WsTcpStreamPtr>(ws_);
      ws.async_close(boost::beast::websocket::close_code::normal, [](boost::beast::error_code) {});
    }
  }

  void TelemetryConnectionImpl::reconnect() {
    if (shutdown_requested_ or is_connected_) {
      return;
    }
    SL_DEBUG(
        log_, "Trying to reconnect in {} seconds", reconnect_timeout_.count());
    scheduler_->schedule([self{shared_from_this()}] { self->connect(); },
                         reconnect_timeout_);
    if (reconnect_timeout_ < kMaxReconnectTimeout) {
      reconnect_timeout_ += kReconnectTimeoutIncrement;
    }
  }

}  // namespace kagome::telemetry
