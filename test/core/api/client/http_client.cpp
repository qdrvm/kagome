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

#include "core/api/client/http_client.hpp"

#include <boost/beast/http/status.hpp>

OUTCOME_CPP_DEFINE_CATEGORY(test, HttpClientError, e) {
  using test::HttpClientError;
  switch (e) {
    case HttpClientError::CONNECTION_FAILED:
      return "connection failed";
    case HttpClientError::HTTP_ERROR:
      return "http error occurred";
    case HttpClientError::NETWORK_ERROR:
      return "network error occurred";
  }
  return "unknown BeastClientError";
}

namespace test {
  using HttpStatus = boost::beast::http::status;

  outcome::result<void> HttpClient::connect(
      boost::asio::ip::tcp::endpoint endpoint) {
    endpoint_ = std::move(endpoint);
    boost::system::error_code ec;
    stream_.connect(endpoint_, ec);
    // TODO (yuraz): ignore error code, log message
    boost::ignore_unused(ec);
    if (ec) {
      return HttpClientError::CONNECTION_FAILED;
    }
    return outcome::success();
  }

  HttpClient::~HttpClient() {
    disconnect();
  }

  void HttpClient::query(std::string_view message,
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
      return callback(HttpClientError::NETWORK_ERROR);
    }

    FlatBuffer buffer{};
    HttpResponse<StringBody> res{};

    // receive response
    boost::beast::http::read(stream_, buffer, res, ec);
    if (ec) {
      return callback(HttpClientError::NETWORK_ERROR);
    }

    if (res.result() != HttpStatus::ok) {
      return callback(HttpClientError::HTTP_ERROR);
    }

    return callback(res.body());
  }

  void HttpClient::disconnect() {
    boost::system::error_code ec{};
    stream_.socket().shutdown(Socket::shutdown_both, ec);
    // TODO (yuraz): log message, ignore error
    boost::ignore_unused(ec);
  }

}  // namespace test
