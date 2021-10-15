/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_OFFCHAIN_HTTPREQUEST
#define KAGOME_OFFCHAIN_HTTPREQUEST

#include <boost/asio/io_context.hpp>

#include "offchain/types.hpp"

namespace kagome::offchain {

  class HttpRequest final {
   public:
    HttpRequest(HttpRequest &&) noexcept = delete;
    HttpRequest(const HttpRequest &) = delete;

    HttpRequest(boost::asio::io_context &io_context,
                RequestId id,
                Method method,
                std::string_view uri,
                common::Buffer meta);

    RequestId id() const;

    HttpStatus status() const;

    Result<Success, Failure> addRequestHeader(std::string_view name,
                                              std::string_view value);

    Result<Success, HttpError> writeRequestBody(
        const common::Buffer &chunk, boost::optional<Timestamp> deadline);

    std::vector<std::pair<std::string, std::string>> getResponseHeaders() const;

    Result<uint32_t, HttpError> readResponseBody(
        common::Buffer &chunk, boost::optional<Timestamp> deadline);

   private:
    boost::asio::io_context &io_context_;
    int16_t id_;
  };

}  // namespace kagome::offchain

#endif  // KAGOME_OFFCHAIN_HTTPREQUEST
