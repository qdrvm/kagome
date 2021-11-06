/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "offchain/impl/uri.hpp"

#include <algorithm>

namespace kagome::offchain {

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
    } else {
      result += "/";
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

    const auto uri_end = uri.end();

    // Schema
    const auto schema_begin = uri.begin();
    auto schema_end = std::find(schema_begin, uri_end, ':');

    if (schema_end != uri_end and uri_end - schema_end > 3
        and std::string_view(schema_end, 3) == "://") {
      result.Schema.assign(schema_begin, schema_end);
      schema_end += 3;

    } else {
      schema_end = uri.begin();
    }

    const auto host_begin = schema_end;
    const auto path_begin = std::find(host_begin, uri_end, '/');
    const auto port_begin = std::find(host_begin, path_begin, ':');

    if (path_begin > port_begin) {
      result.Host.assign(host_begin, port_begin);
      result.Port.assign(port_begin + 1, path_begin);
    } else {
      result.Host.assign(host_begin, path_begin);
    }

    const auto path_end = std::find_if(
        path_begin, uri_end, [](auto ch) { return ch == '?' or ch == '#'; });

    result.Path.assign(path_begin, path_end);

    const auto query_begin = path_end == uri_end ? uri_end : (path_end + 1);
    const auto query_end = std::find(query_begin, uri_end, '#');

    result.Query.assign(query_begin, query_end);

    const auto fragm_begin = query_end == uri_end ? uri_end : (query_end + 1);
    const auto fragm_end = uri_end;

    result.Fragment.assign(fragm_begin, fragm_end);

    return result;
  }
}  // namespace kagome::offchain
