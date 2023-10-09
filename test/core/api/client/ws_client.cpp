/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "core/api/client/ws_client.hpp"

#include <boost/beast/http/status.hpp>

OUTCOME_CPP_DEFINE_CATEGORY(test, WsClientError, e) {
  using test::WsClientError;
  switch (e) {
    case WsClientError::CONNECTION_FAILED:
      return "connection failed";
    case WsClientError::WEBSOCKET_ERROR:
      return "websocket error occurred";
    case WsClientError::HTTP_ERROR:
      return "http error occurred";
    case WsClientError::NETWORK_ERROR:
      return "network error occurred";
  }
  return "unknown BeastClientError";
}

namespace test {
  using HttpStatus = boost::beast::http::status;

  outcome::result<void> test::WsClient::connect(
      boost::asio::ip::tcp::endpoint endpoint) {
    endpoint_ = std::move(endpoint);
    boost::system::error_code ec;

    stream_.next_layer().connect(endpoint_, ec);
    if (ec) {
      return WsClientError::CONNECTION_FAILED;
    }

    stream_.handshake(endpoint_.address().to_string(), "/", ec);
    if (ec) {
      return WsClientError::WEBSOCKET_ERROR;
    }

    return outcome::success();
  }

  WsClient::~WsClient() {
    disconnect();
  }

  void test::WsClient::query(std::string_view message,
                             std::function<QueryCallback> &&callback) {
    FlatBuffer buffer{};

    boost::system::error_code ec{};

    auto x = buffer.prepare(message.size());
    std::copy(message.begin(),
              message.end(),
              reinterpret_cast<char *>(x.data()));  // NOLINT
    buffer.commit(message.size());

    stream_.write(buffer.data(), ec);
    buffer.consume(message.size());
    if (ec) {
      return callback(WsClientError::NETWORK_ERROR);
    }

    stream_.read(buffer, ec);
    if (ec) {
      return callback(WsClientError::NETWORK_ERROR);
    }

    return callback(boost::beast::buffers_to_string(buffer.data()));
  }

  void WsClient::disconnect() {
    boost::system::error_code ec{};
    stream_.close("done", ec);
    stream_.next_layer().socket().shutdown(Socket::shutdown_both, ec);
    // TODO (yuraz): log message, ignore error
    boost::ignore_unused(ec);
  }

}  // namespace test
