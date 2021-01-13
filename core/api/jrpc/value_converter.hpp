/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_API_EXTRINSIC_RESPONSE_VALUE_CONVERTER_HPP
#define KAGOME_CORE_API_EXTRINSIC_RESPONSE_VALUE_CONVERTER_HPP

#include <vector>

#include <jsonrpc-lean/value.h>
#include <boost/range/adaptor/transformed.hpp>

#include "common/blob.hpp"
#include "common/hexutil.hpp"
#include "common/visitor.hpp"
#include "primitives/block_data.hpp"
#include "primitives/block_header.hpp"
#include "primitives/digest.hpp"
#include "primitives/event_types.hpp"
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
  inline jsonrpc::Value makeValue(const primitives::BlockData &);
  inline jsonrpc::Value makeValue(const primitives::BlockHeader &);
  inline jsonrpc::Value makeValue(const primitives::Justification &);

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
    return common::hex_lower_0x(scale::encode(v.data).value());
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

  template <class T>
  inline jsonrpc::Value makeValue(const boost::optional<T> &val) {
    if (!val) return jsonrpc::Value{};

    return makeValue(*val);
  }

  inline jsonrpc::Value makeValue(const primitives::Api &val) {
    using VectorType = jsonrpc::Value::Array;
    VectorType data;

    data.reserve(2);
    data.emplace_back(makeValue(val.first));
    data.emplace_back(makeValue(val.second));

    return data;
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
    data["transactionVersion"] = makeValue(val.transaction_version);

    jArray apis;
    std::transform(val.apis.begin(),
                   val.apis.end(),
                   std::back_inserter(apis),
                   [](const auto &api) {
                     jArray api_data;
                     api_data.emplace_back(jString(hex_lower_0x(api.first)));
                     api_data.emplace_back(makeValue(api.second));
                     return api_data;
                   });

    data["apis"] = std::move(apis);
    return data;
  }

  inline jsonrpc::Value makeValue(const primitives::BlockHeader &val) {
    using jStruct = jsonrpc::Value::Struct;
    using jArray = jsonrpc::Value::Array;

    jStruct data;
    std::stringstream stream;
    stream << std::hex << val.number;
    std::string result("0x" + stream.str());
    data["parentHash"] = makeValue(common::hex_lower_0x(val.parent_hash));
    data["number"] = makeValue(result);
    data["stateRoot"] = makeValue(common::hex_lower_0x(val.state_root));
    data["extrinsicsRoot"] =
        makeValue(common::hex_lower_0x(val.extrinsics_root));

    jArray logs;
    logs.reserve(val.digest.size());
    for (const auto &d : val.digest) {
      logs.emplace_back(makeValue(d));
    }

    jStruct digest;
    digest["logs"] = std::move(logs);

    data["digest"] = std::move(digest);
    return data;
  }

  inline jsonrpc::Value makeValue(const primitives::Justification &val) {
    return makeValue(val.data);
  }

  inline jsonrpc::Value makeValue(const primitives::BlockData &val) {
    using jStruct = jsonrpc::Value::Struct;

    jStruct block;
    block["extrinsics"] = makeValue(val.body);
    block["header"] = makeValue(val.header);

    jStruct data;
    data["block"] = std::move(block);
    data["justification"] = makeValue(val.justification);
    return data;
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

  inline jsonrpc::Value makeValue(
      const primitives::events::ExtrinsicLifecycleEvent &event) {
    using jStruct = jsonrpc::Value::Struct;
    return visit_in_place(
        event.params,
        [&event](boost::none_t) -> jsonrpc::Value {
          switch (event.type) {
            case primitives::events::ExtrinsicEventType::FUTURE:
              return "future";
            case primitives::events::ExtrinsicEventType::READY:
              return "ready";
            case primitives::events::ExtrinsicEventType::INVALID:
              return "invalid";
            case primitives::events::ExtrinsicEventType::DROPPED:
              return "dropped";
            default:
              BOOST_UNREACHABLE_RETURN({});
          }
        },
        [](const primitives::events::BroadcastEventParams &params)
            -> jsonrpc::Value {
          jsonrpc::Value::Array peers;
          peers.reserve(params.peers.size());
          std::transform(
              params.peers.cbegin(),
              params.peers.cend(),
              peers.begin(),
              [](const auto &peer_id) { return makeValue(peer_id.toHex()); });
          return jStruct{std::pair{"broadcast", std::move(peers)}};
        },
        [](const primitives::events::InBlockEventParams &params) {
          return jStruct{std::pair{
              "inBlock", makeValue(common::hex_lower_0x(params.block))}};
        },
        [](const primitives::events::RetractedEventParams &params) {
          return jStruct{std::pair{
              "retracted",
              makeValue(common::hex_lower_0x(params.retracted_block))}};
        },
        [](const primitives::events::FinalityTimeoutEventParams &params) {
          return jStruct{
              std::pair{"finalityTimeout",
                        makeValue(common::hex_lower_0x(params.block))}};
        },
        [](const primitives::events::FinalizedEventParams &params) {
          return jStruct{std::pair{
              "finalized", makeValue(common::hex_lower_0x(params.block))}};
        },
        [](const primitives::events::UsurpedEventParams &params) {
          return jStruct{std::pair{
              "usurped",
              makeValue(common::hex_lower_0x(params.transaction_hash))}};
        });
  }
}  // namespace kagome::api

#endif  // KAGOME_CORE_API_EXTRINSIC_RESPONSE_VALUE_CONVERTER_HPP
