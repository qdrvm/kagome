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
#include "primitives/version.hpp"

namespace kagome::api {
  inline jsonrpc::Value makeValue(common::Hash256 const &);
  inline jsonrpc::Value makeValue(common::Buffer const &);
  inline jsonrpc::Value makeValue(primitives::Extrinsic const &);
  inline jsonrpc::Value makeValue(primitives::Version const &);
  inline jsonrpc::Value makeValue(uint32_t const &);
  inline jsonrpc::Value makeValue(primitives::Api const &);

  template <size_t S>
  inline jsonrpc::Value makeValue(const common::Blob<S> &);
  template <typename T>
  inline jsonrpc::Value makeValue(T const &);
  template <class T>
  inline jsonrpc::Value makeValue(const std::vector<T> &);
  template <class T1, class T2>
  inline jsonrpc::Value makeValue(const boost::variant<T1, T2> &);

  template <typename T>
  inline jsonrpc::Value makeValue(T const &val) {
    return jsonrpc::Value(val);
  }

  template <size_t S>
  inline jsonrpc::Value makeValue(common::Blob<S> const &val) {
    return std::vector<uint8_t>{val.begin(), val.end()};
  }

  inline jsonrpc::Value makeValue(uint32_t const &val) {
    return makeValue(static_cast<int64_t>(val));
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
  inline jsonrpc::Value makeValue(const std::vector<T> &v) {
    jsonrpc::Value::Array value{};
    value.reserve(v.size());
    for (auto &item : v) {
      value.push_back(std::move(makeValue(item)));
    }
    return value;
  }

  inline jsonrpc::Value makeValue(const primitives::Api &val) {
    using VectorType = jsonrpc::Value::Array;
    VectorType data;

    data.reserve(2);
    data.emplace_back(makeValue(val.first));
    data.emplace_back(makeValue(val.second));

    return std::move(data);
  }

  inline jsonrpc::Value makeValue(const primitives::Version &val) {
    using jStruct = jsonrpc::Value::Struct;
    jStruct data;
    data["authoringVersion"] = makeValue(val.authoring_version);

    data["specName"] = makeValue(val.spec_name);
    data["implName"] = makeValue(val.impl_name);

    data["specVersion"] = makeValue(val.spec_version);
    data["implVersion"] = makeValue(val.impl_version);

    data["apis"] = makeValue(val.apis);
    return std::move(data);
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
