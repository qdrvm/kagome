/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "offchain/impl/uri.hpp"

#include <algorithm>

namespace kagome::offchain {

  Uri::Uri(Uri &&other) noexcept {
    auto uri = std::move(other.uri_);
    auto error = std::move(other.error_);
    *this = other;
    other.reset();
    uri_ = std::move(uri);
    error_ = std::move(error);
  }

  Uri &Uri::operator=(Uri &&other) noexcept {
    auto uri = std::move(other.uri_);
    auto error = std::move(other.error_);
    *this = other;
    other.reset();
    uri_ = std::move(uri);
    error_ = std::move(error);
    return *this;
  }

  Uri Uri::Parse(std::string_view uri) {
    Uri result;

    if (uri.empty()) {
      return result;
    }

    result.uri_.assign(uri);
    uri = result.uri_;
    result.error_.reset();

    const auto uri_end = uri.cend();

    // Schema
    const auto schema_begin = uri.cbegin();
    auto schema_end = std::find(schema_begin, uri_end, ':');

    if (schema_end == uri_end or uri_end - schema_end <= 3
        or std::string_view(schema_end, 3) != "://") {
      schema_end = uri.cbegin();
    }
    result.schema_offset_ = schema_begin - uri.cbegin();
    result.schema_size_ = schema_end - schema_begin;

    if (std::find_if_not(
            schema_begin, schema_end, [](auto ch) { return std::isalpha(ch); })
        != schema_end) {
      if (not result.error_.has_value()) {
        result.error_.emplace("Invalid schema");
      }
    }

    // Host
    const auto host_begin =
        schema_end + (std::string_view(schema_end, 3) == "://" ? 3 : 0);
    const auto host_end = std::find_if(host_begin, uri_end, [](auto ch) {
      return ch == ':' or ch == '/' or ch == '?' or ch == '#';
    });
    result.host_offset_ = host_begin - uri.cbegin();
    result.host_size_ = host_end - host_begin;
    if (std::find_if_not(
            host_begin,
            host_end,
            [](auto ch) { return std::isalnum(ch) or ch == '.' or ch == '-'; })
            != host_end
        or host_begin == host_end) {
      if (not result.error_.has_value()) {
        result.error_.emplace("Invalid hostname");
      }
    }

    // Port
    const auto port_begin = host_end + (*host_end == ':' ? 1 : 0);
    const auto port_end = std::find_if(port_begin, uri_end, [](auto ch) {
      return ch == '/' or ch == '?' or ch == '#';
    });

    result.port_offset_ = port_begin - uri.cbegin();
    result.port_size_ = port_end - port_begin;

    if (std::find_if_not(
            port_begin, port_end, [](auto ch) { return std::isdigit(ch); })
            != port_end
        or (result.port_size_ == 0 and *host_end == ':')
        or (result.port_size_ == 1 and result.port() == "0")
        or (result.port_size_ == 5 and result.port() > "65535")
        or result.port_size_ > 5) {
      if (not result.error_.has_value()) {
        result.error_.emplace("Invalid port");
      }
    }

    // Path
    const auto path_begin = port_end;
    const auto path_end = std::find_if(
        path_begin, uri_end, [](auto ch) { return ch == '?' or ch == '#'; });

    result.path_offset_ = path_begin - uri.cbegin();
    result.path_size_ = path_end - path_begin;

    // Query
    const auto query_begin = path_end + (*path_end == '?' ? 1 : 0);
    const auto query_end = std::find(query_begin, uri_end, '#');

    result.query_offset_ = query_begin - uri.cbegin();
    result.query_size_ = query_end - query_begin;

    // Fragment
    const auto fragment_begin = query_end + (*query_end == '#' ? 1 : 0);
    const auto fragment_end = uri_end;

    result.fragment_offset_ = fragment_begin - uri.cbegin();
    result.fragment_size_ = fragment_end - fragment_begin;

    return result;
  }
}  // namespace kagome::offchain
