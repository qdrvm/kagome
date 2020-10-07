/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_API_EXTRINSIC_RESPONSE_VALUE_CONVERTER_HPP
#define KAGOME_CORE_API_EXTRINSIC_RESPONSE_VALUE_CONVERTER_HPP

#include <jsonrpc-lean/value.h>
#include <vector>

#include "common/blob.hpp"
#include "common/visitor.hpp"
#include "primitives/block_header.hpp"
#include "primitives/digest.hpp"
#include "primitives/extrinsic.hpp"
#include "primitives/version.hpp"
#include "scale/scale.hpp"

namespace kagome::api {
  inline jsonrpc::Value makeValue(const common::Hash256 &);
  inline jsonrpc::Value makeValue(const common::Buffer &);
  inline jsonrpc::Value makeValue(const primitives::Extrinsic &);
  inline jsonrpc::Value makeValue(const primitives::Version &);
  inline jsonrpc::Value makeValue(const uint32_t &);
  inline jsonrpc::Value makeValue(const uint64_t &);
  inline jsonrpc::Value makeValue(const primitives::Api &);
  inline jsonrpc::Value makeValue(const primitives::DigestItem &);

  inline jsonrpc::Value makeValue(const std::nullptr_t &) {
    return jsonrpc::Value();
  }
  inline jsonrpc::Value makeValue(const std::nullopt_t &) {
    return jsonrpc::Value();
  }
  inline jsonrpc::Value makeValue(const boost::none_t &) {
    return jsonrpc::Value();
  }

  template <size_t S>
  inline jsonrpc::Value makeValue(const common::Blob<S> &);
  template <typename T>
  inline jsonrpc::Value makeValue(const T &);
  template <class T>
  inline jsonrpc::Value makeValue(const std::vector<T> &);
  template <typename... Ts>
  inline jsonrpc::Value makeValue(const boost::variant<Ts...> &v);
  template <typename T>
  inline jsonrpc::Value makeValue(const std::reference_wrapper<T> &v);

  template <typename T>
  inline jsonrpc::Value makeValue(const T &val) {
    return jsonrpc::Value(val);
  }

  template <size_t S>
  inline jsonrpc::Value makeValue(const common::Blob<S> &val) {
    return std::vector<uint8_t>{val.begin(), val.end()};
  }

  inline jsonrpc::Value makeValue(const primitives::DigestItem &val) {
    auto result = scale::encode(val);
    if (result.has_error())
      throw jsonrpc::InternalErrorFault("Unable to encode arguments.");

    return jsonrpc::Value(common::hex_lower_0x(result.value()));
  }

  inline jsonrpc::Value makeValue(const uint32_t &val) {
    return makeValue(static_cast<int64_t>(val));
  }

  inline jsonrpc::Value makeValue(const uint64_t &val) {
    return makeValue(static_cast<int64_t>(val));
  }

  inline jsonrpc::Value makeValue(const common::Hash256 &v) {
    return std::vector<uint8_t>{v.begin(), v.end()};
  }

  inline jsonrpc::Value makeValue(const common::Buffer &v) {
    static const std::string prefix("0x");
    return prefix + v.toHex();
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
    using jArray = jsonrpc::Value::Array;
    using jString = jsonrpc::Value::String;

    jStruct data;
    data["authoringVersion"] = makeValue(val.authoring_version);

    data["specName"] = makeValue(val.spec_name);
    data["implName"] = makeValue(val.impl_name);

    data["specVersion"] = makeValue(val.spec_version);
    data["implVersion"] = makeValue(val.impl_version);

    jArray apis;
    std::transform(val.apis.begin(),
                   val.apis.end(),
                   std::back_inserter(apis),
                   [](const auto &api) {
                     jArray api_data;
                     api_data.emplace_back(jString(api.first.toHex()));
                     api_data.emplace_back(makeValue(api.second));
                     return api_data;
                   });

    data["apis"] = std::move(apis);
    return std::move(data);
  }

  inline jsonrpc::Value makeValue(const primitives::BlockHeader &val) {
    using jStruct = jsonrpc::Value::Struct;
    using jArray = jsonrpc::Value::Array;

    jStruct data;
    data["parentHash"] = makeValue(val.parent_hash);
    data["number"] = makeValue(val.number);
    data["stateRoot"] = makeValue(val.state_root);
    data["extrinsicsRoot"] = makeValue(val.extrinsics_root);

    jArray logs;
    logs.reserve(val.digest.size());
    for (auto &d : val.digest) {
      logs.emplace_back(makeValue(d));
    }

    jStruct digest;
    digest["logs"] = std::move(logs);

    data["digest"] = std::move(digest);
    return std::move(data);
  }

  template <typename... Ts>
  inline jsonrpc::Value makeValue(const boost::variant<Ts...> &v) {
    return visit_in_place(v,
                          [](const auto &value) { return makeValue(value); });
  }

  template <typename T>
  inline jsonrpc::Value makeValue(const std::reference_wrapper<T> &v) {
    return makeValue(v.get());
  }
}  // namespace kagome::api

#endif  // KAGOME_CORE_API_EXTRINSIC_RESPONSE_VALUE_CONVERTER_HPP
