/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "offchain/impl/uri.hpp"

#include <algorithm>

namespace kagome::offchain {

  Uri Uri::Parse(std::string_view uri) {
    Uri result;

    if (uri.empty()) {
      return result;
    }

    result.uri_.assign(uri);
    uri = result.uri_;

    const auto uri_end = uri.end();

    // Schema
    const auto schema_begin = uri.begin();
    auto schema_end = std::find(schema_begin, uri_end, ':');

    if (schema_end != uri_end and uri_end - schema_end > 3
        and std::string_view(schema_end, 3) == "://") {
      result.Schema = std::string_view(schema_begin, schema_end - schema_begin);
      schema_end += 3;

    } else {
      schema_end = uri.begin();
    }

    const auto host_begin = schema_end;
    const auto path_begin = std::find(host_begin, uri_end, '/');
    const auto port_begin = std::find(host_begin, path_begin, ':') + 1;

    if (path_begin > port_begin) {
      result.Host = std::string_view(host_begin, port_begin - host_begin);
      result.Port = std::string_view(port_begin, path_begin - port_begin);
    } else {
      result.Host = std::string_view(host_begin, path_begin - host_begin);
      result.Port = std::string_view(port_begin, 0);
    }

    const auto path_end = std::find_if(
        path_begin, uri_end, [](auto ch) { return ch == '?' or ch == '#'; });

    result.Path = std::string_view(path_begin, path_end - path_begin);

    const auto query_begin = path_end == uri_end ? uri_end : (path_end + 1);
    const auto query_end = std::find(query_begin, uri_end, '#');

    result.Query = std::string_view(query_begin, query_end - query_begin);

    const auto fragm_begin = query_end == uri_end ? uri_end : (query_end + 1);
    const auto fragm_end = uri_end;

    result.Fragment = std::string_view(fragm_begin, fragm_end - fragm_begin);

    return result;
  }
}  // namespace kagome::offchain
