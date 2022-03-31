/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "common/uri.hpp"

#include <algorithm>

namespace kagome::common {

  std::string Uri::toString() const {
    std::string result;
    if (not Schema.empty()) {
      result += Schema;
      result += ":";
    }
    if (not Host.empty()) {
      if (not Schema.empty()) {
        result += "//";
      }
      result += Host;
      if (not Port.empty()) {
        result += ":";
        result += Port;
      }
    }
    if (not Path.empty()) {
      result += Path;
    }
    if (not Query.empty()) {
      result += "?";
      result += Query;
    }
    if (not Fragment.empty()) {
      result += "#";
      result += Fragment;
    }
    return result;
  }

  Uri Uri::Parse(std::string_view uri) {
    Uri result;

    if (uri.empty()) {
      return result;
    }

    result.error_.reset();

    const auto uri_end = uri.cend();

    // Schema
    const auto schema_begin = uri.cbegin();
    auto schema_end = std::find(schema_begin, uri_end, ':');

    if (schema_end == uri_end or uri_end - schema_end <= 3
        or std::string_view(schema_end, 3) != "://") {
      schema_end = uri.cbegin();
    }
    result.Schema.assign(schema_begin, schema_end);

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
    result.Host.assign(host_begin, host_end);
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

    result.Port.assign(port_begin, port_end);

    if (std::find_if_not(
            port_begin, port_end, [](auto ch) { return std::isdigit(ch); })
            != port_end
        or (result.Port.empty() and *host_end == ':') or (result.Port == "0")
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

    result.Path.assign(path_begin, path_end);

    // Query
    const auto query_begin = path_end + (*path_end == '?' ? 1 : 0);
    const auto query_end = std::find(query_begin, uri_end, '#');

    result.Query.assign(query_begin, query_end);

    // Fragment
    const auto fragment_begin = query_end + (*query_end == '#' ? 1 : 0);
    const auto fragment_end = uri_end;

    result.Fragment.assign(fragment_begin, fragment_end);

    return result;
  }
}  // namespace kagome::common
