/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "core/api/client/api_client.hpp"

#include <boost/beast/http/status.hpp>

enum class BeastClientError {
  CONNECTION_FAILED = 1,
  NOT_CONNECTED,
  HTTP_ERROR
};

OUTCOME_CPP_DEFINE_CATEGORY(test, ApiClientError, e) {
  using test::ApiClientError;
  switch (e) {
    case ApiClientError::CONNECTION_FAILED:
      return "connection failed";
    case ApiClientError::HTTP_ERROR:
      return "http error occurred";
    case ApiClientError::NETWORK_ERROR:
      return "network error occurred";
  }
  return "unknown BeastClientError";
}

namespace test {
  using HttpStatus = boost::beast::http::status;

  outcome::result<void> test::ApiClient::connect(
      boost::asio::ip::tcp::endpoint endpoint) {
    endpoint_ = std::move(endpoint);
    boost::system::error_code ec;
    stream_.connect(endpoint_, ec);
    // TODO (yuraz): ignore error code, log message
    boost::ignore_unused(ec);
    if (ec) {
      return ApiClientError::CONNECTION_FAILED;
    }
    return outcome::success();
  }

  ApiClient::~ApiClient() {
    disconnect();
  }

  void test::ApiClient::query(std::string_view message,
                              std::function<QueryCallback> &&callback) {
    // send request
    HttpRequest<StringBody> req(HttpMethods::post, "/", 11);
    auto host = endpoint_.address().to_string() + ":"
                + std::to_string(endpoint_.port());
    req.set(HttpField::host, host);
    req.set(HttpField::user_agent, kUserAgent);
    req.set(HttpField::content_type, "text/html");
    req.set(HttpField::content_length, message.length());
    req.body().assign(message);

    boost::system::error_code ec{};

    // Send the HTTP request to the remote host
    boost::beast::http::write(stream_, req, ec);
    if (ec) {
      return callback(ApiClientError::NETWORK_ERROR);
    }

    FlatBuffer buffer{};
    HttpResponse<StringBody> res{};

    // receive response
    boost::beast::http::read(stream_, buffer, res, ec);
    if (ec) {
      return callback(ApiClientError::NETWORK_ERROR);
    }

    if (res.result() != HttpStatus::ok) {
      return callback(ApiClientError::HTTP_ERROR);
    }

    return callback(res.body());
  }

  void ApiClient::disconnect() {
    boost::system::error_code ec{};
    stream_.socket().shutdown(Socket::shutdown_both, ec);
    // TODO (yuraz): log message, ignore error
    boost::ignore_unused(ec);
  }

}  // namespace test
