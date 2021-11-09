/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "offchain/impl/uri.hpp"

#include <algorithm>

namespace kagome::offchain {

  Uri::Uri(const Uri &other) : uri_(other.uri_), error_(other.error_) {
    auto offset = uri_.data() - other.uri_.data();
    Schema = {other.Schema.data() + offset, other.Schema.size()};
    Host = {other.Host.data() + offset, other.Host.size()};
    Port = {other.Port.data() + offset, other.Port.size()};
    Path = {other.Path.data() + offset, other.Path.size()};
    Query = {other.Query.data() + offset, other.Query.size()};
    Fragment = {other.Fragment.data() + offset, other.Fragment.size()};
  }

  Uri::Uri(Uri &&other) noexcept
      : uri_(std::move(other.uri_)),
        error_(std::move(other.error_)),
        Schema(other.Schema),
        Host(other.Host),
        Port(other.Port),
        Path(other.Path),
        Query(other.Query),
        Fragment(other.Fragment) {
    other.Schema = {other.uri_.data(), 0};
    other.Host = {other.uri_.data(), 0};
    other.Port = {other.uri_.data(), 0};
    other.Path = {other.uri_.data(), 0};
    other.Query = {other.uri_.data(), 0};
    other.Fragment = {other.uri_.data(), 0};
    other.error_.emplace("Is not initialized");
  }

  Uri &Uri::operator=(const Uri &other) {
    uri_ = other.uri_;
    error_ = other.error_;

    auto offset = uri_.data() - other.uri_.data();
    Schema = {other.Schema.data() + offset, other.Schema.size()};
    Host = {other.Host.data() + offset, other.Host.size()};
    Port = {other.Port.data() + offset, other.Port.size()};
    Path = {other.Path.data() + offset, other.Path.size()};
    Query = {other.Query.data() + offset, other.Query.size()};
    Fragment = {other.Fragment.data() + offset, other.Fragment.size()};

    return *this;
  }

  Uri &Uri::operator=(Uri &&other) noexcept {
    uri_ = std::move(other.uri_);

    error_ = std::move(other.error_);
    other.error_.emplace("Is not initialized");

    Schema = other.Schema;
    Host = other.Host;
    Port = other.Port;
    Path = other.Path;
    Query = other.Query;
    Fragment = other.Fragment;

    other.Schema = {other.uri_.data(), 0};
    other.Host = {other.uri_.data(), 0};
    other.Port = {other.uri_.data(), 0};
    other.Path = {other.uri_.data(), 0};
    other.Query = {other.uri_.data(), 0};
    other.Fragment = {other.uri_.data(), 0};

    return *this;
  }

  Uri Uri::Parse(std::string uri_arg) {
    Uri result;

    if (uri_arg.empty()) {
      return result;
    }

    result.uri_ = std::move(uri_arg);
    std::string_view uri = result.uri_;
    result.error_.reset();

    const auto uri_end = uri.cend();

    // Schema
    const auto schema_begin = uri.cbegin();
    auto schema_end = std::find(schema_begin, uri_end, ':');

    if (schema_end == uri_end or uri_end - schema_end <= 3
        or std::string_view(schema_end, 3) != "://") {
      schema_end = uri.cbegin();
    }
    result.Schema = std::string_view(schema_begin, schema_end - schema_begin);

    if (std::find_if_not(result.Schema.begin(),
                         result.Schema.end(),
                         [](auto ch) { return std::isalpha(ch); })
        != result.Schema.end()) {
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
    result.Host = std::string_view(host_begin, host_end - host_begin);
    if (std::find_if_not(
            result.Host.begin(),
            result.Host.end(),
            [](auto ch) { return std::isalnum(ch) or ch == '.' or ch == '-'; })
            != result.Host.end()
        or result.Host.empty()) {
      if (not result.error_.has_value()) {
        result.error_.emplace("Invalid hostname");
      }
    }

    // Port
    const auto port_begin = host_end + (*host_end == ':' ? 1 : 0);
    const auto port_end = std::find_if(port_begin, uri_end, [](auto ch) {
      return ch == '/' or ch == '?' or ch == '#';
    });

    result.Port = std::string_view(port_begin, port_end - port_begin);

    if ((result.Port.size() == 1 and result.Port == "0")
        or std::find_if_not(result.Port.begin(),
                            result.Port.end(),
                            [](auto ch) { return std::isdigit(ch); })
               != result.Port.end()
        or (result.Port.empty() and *host_end == ':')
        or (result.Port.size() == 1 and result.Port == "0")
        or (result.Port.size() == 5 and result.Port > "65535")
        or result.Port.size() > 5) {
      if (not result.error_.has_value()) {
        result.error_.emplace("Invalid port");
      }
    }

    // Path
    const auto path_begin = port_end;
    const auto path_end = std::find_if(
        path_begin, uri_end, [](auto ch) { return ch == '?' or ch == '#'; });

    result.Path = std::string_view(path_begin, path_end - path_begin);

    // Query
    const auto query_begin = path_end + (*path_end == '?' ? 1 : 0);
    const auto query_end = std::find(query_begin, uri_end, '#');

    result.Query = std::string_view(query_begin, query_end - query_begin);

    // Fragment
    const auto fragm_begin = query_end + (*query_end == '#' ? 1 : 0);
    const auto fragm_end = uri_end;

    result.Fragment = std::string_view(fragm_begin, fragm_end - fragm_begin);

    return result;
  }
}  // namespace kagome::offchain
