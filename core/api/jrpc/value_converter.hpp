/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_API_EXTRINSIC_RESPONSE_VALUE_CONVERTER_HPP
#define KAGOME_CORE_API_EXTRINSIC_RESPONSE_VALUE_CONVERTER_HPP

#include <vector>

#include <jsonrpc-lean/value.h>
#include "common/blob.hpp"
#include "common/visitor.hpp"
#include "primitives/extrinsic.hpp"

namespace kagome::api {
  inline jsonrpc::Value makeValue(std::string v) {
    return std::move(v);
  }

  inline jsonrpc::Value makeValue(const common::Hash256 &v) {
    return std::vector<uint8_t>{v.begin(), v.end()};
  }

  inline jsonrpc::Value makeValue(const common::Buffer &v) {
    return v.toVector();
  }

  inline jsonrpc::Value makeValue(const primitives::Extrinsic &v) {
    return v.data.toHex();
  }

  template <class T>
  jsonrpc::Value makeValue(const std::vector<T> &v) {
    jsonrpc::Value::Array value{};
    value.reserve(v.size());
    for (auto &item : v) {
      value.push_back(std::move(makeValue(item)));
    }
    return value;
  }

  template <class T1, class T2>
  inline jsonrpc::Value makeValue(const boost::variant<T1, T2> &v) {
    return kagome::visit_in_place(
        v,
        [](const T1 &value) { return makeValue(value); },
        [](const T2 &value) { return makeValue(value); });
  }

}  // namespace kagome::api

#endif  // KAGOME_CORE_API_EXTRINSIC_RESPONSE_VALUE_CONVERTER_HPP
