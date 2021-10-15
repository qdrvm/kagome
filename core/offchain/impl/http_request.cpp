/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "offchain/impl/http_request.hpp"

namespace kagome::offchain {
  HttpRequest::HttpRequest(boost::asio::io_context &io_context,
                           RequestId id,
                           Method method,
                           std::string_view uri,
                           common::Buffer meta)
      : io_context_(io_context), id_(id) {}

  RequestId HttpRequest::id() const {
    return id_;
  }

  HttpStatus HttpRequest::status() const {
    // TODO(xDimon): Need to implement it
    throw std::runtime_error(
        "Method HttpRequest::status is not implemented yet");
    return {};
  }

  Result<Success, Failure> HttpRequest::addRequestHeader(
      std::string_view name, std::string_view value) {
    // TODO(xDimon): Need to implement it
    throw std::runtime_error(
        "Method HttpRequest::addRequestHeader is not implemented yet");
    return Result<Success, Failure>();
  }

  Result<Success, HttpError> HttpRequest::writeRequestBody(
      const common::Buffer &chunk, boost::optional<Timestamp> deadline) {
    // TODO(xDimon): Need to implement it
    throw std::runtime_error(
        "Method HttpRequest::writeRequestBody is not implemented yet");
    return Result<Success, HttpError>();
  }

  std::vector<std::pair<std::string, std::string>>
  HttpRequest::getResponseHeaders() const {
    // TODO(xDimon): Need to implement it
    throw std::runtime_error(
        "Method HttpRequest::getResponseHeaders is not implemented yet");
    return std::vector<std::pair<std::string, std::string>>();
  }

  Result<uint32_t, HttpError> HttpRequest::readResponseBody(
      common::Buffer &chunk, boost::optional<Timestamp> deadline) {
    // TODO(xDimon): Need to implement it
    throw std::runtime_error(
        "Method HttpRequest::readResponseBody is not implemented yet");
    return Result<uint32_t, HttpError>();
  }
}  // namespace kagome::offchain
