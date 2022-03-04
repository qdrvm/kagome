/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_API_EXTRINSIC_RESPONSE_VALUE_CONVERTER_HPP
#define KAGOME_CORE_API_EXTRINSIC_RESPONSE_VALUE_CONVERTER_HPP

#include <vector>

#include <jsonrpc-lean/value.h>
#include <boost/range/adaptor/transformed.hpp>

#include "api/service/state/state_api.hpp"
#include "common/blob.hpp"
#include "common/hexutil.hpp"
#include "common/visitor.hpp"
#include "primitives/block_data.hpp"
#include "primitives/block_header.hpp"
#include "primitives/digest.hpp"
#include "primitives/event_types.hpp"
#include "primitives/extrinsic.hpp"
#include "primitives/rpc_methods.hpp"
#include "primitives/runtime_dispatch_info.hpp"
#include "primitives/version.hpp"
#include "scale/scale.hpp"

namespace kagome::api {

  namespace {
    using jString = jsonrpc::Value::String;
    using jArray = jsonrpc::Value::Array;
    using jStruct = jsonrpc::Value::Struct;
  }  // namespace

  inline jsonrpc::Value makeValue(const uint32_t &);
  inline jsonrpc::Value makeValue(const uint64_t &);
  inline jsonrpc::Value makeValue(const std::nullptr_t &);
  inline jsonrpc::Value makeValue(const std::nullopt_t &);
  inline jsonrpc::Value makeValue(const std::nullopt_t &);

  template <typename T>
  inline jsonrpc::Value makeValue(const std::reference_wrapper<T> &v);

  template <typename T>
  inline jsonrpc::Value makeValue(const std::optional<T> &val);

  template <typename... Ts>
  inline jsonrpc::Value makeValue(const boost::variant<Ts...> &v);

  template <typename T1, typename T2>
  inline jsonrpc::Value makeValue(const std::pair<T1, T2> &val);

  template <typename T>
  inline jsonrpc::Value makeValue(const std::vector<T> &);

  template <size_t N>
  inline jsonrpc::Value makeValue(const common::Blob<N> &);

  inline jsonrpc::Value makeValue(const common::Buffer &);
  inline jsonrpc::Value makeValue(common::BufferView);
  inline jsonrpc::Value makeValue(const primitives::Extrinsic &);
  inline jsonrpc::Value makeValue(const primitives::RuntimeDispatchInfo &v);
  inline jsonrpc::Value makeValue(const primitives::DigestItem &);
  inline jsonrpc::Value makeValue(const primitives::BlockData &);
  inline jsonrpc::Value makeValue(const primitives::BlockHeader &);
  inline jsonrpc::Value makeValue(const primitives::Version &);
  inline jsonrpc::Value makeValue(const primitives::Justification &);
  inline jsonrpc::Value makeValue(const primitives::RpcMethods &);

  inline jsonrpc::Value makeValue(const uint32_t &val) {
    return static_cast<int64_t>(val);
  }

  inline jsonrpc::Value makeValue(const uint64_t &val) {
    return static_cast<int64_t>(val);
  }

  inline jsonrpc::Value makeValue(const std::nullptr_t &) {
    return {};
  }

  inline jsonrpc::Value makeValue(const std::nullopt_t &) {
    return {};
  }

  template <typename T>
  inline jsonrpc::Value makeValue(const T &val) {
    jsonrpc::Value ret(val);
    return ret;
  }

  template <typename T>
  inline jsonrpc::Value makeValue(std::remove_reference_t<T> &&val) {
    return makeValue(val);
  }

  template <typename T>
  inline jsonrpc::Value makeValue(const std::reference_wrapper<T> &v) {
    return makeValue(v.get());
  }

  template <typename T>
  inline jsonrpc::Value makeValue(const std::optional<T> &val) {
    if (!val) return {};
    return makeValue(*val);
  }

  template <typename... Ts>
  inline jsonrpc::Value makeValue(const boost::variant<Ts...> &v) {
    return visit_in_place(v,
                          [](const auto &value) { return makeValue(value); });
  }

  template <typename T1, typename T2>
  inline jsonrpc::Value makeValue(const std::pair<T1, T2> &val) {
    jArray data;

    data.reserve(2);
    data.emplace_back(makeValue(val.first));
    data.emplace_back(makeValue(val.second));

    return data;
  }

  template <typename T>
  inline jsonrpc::Value makeValue(const std::vector<T> &v) {
    jArray value;

    value.reserve(v.size());
    for (auto &item : v) {
      value.emplace_back(makeValue(item));
    }

    return value;
  }

  template <size_t N>
  inline jsonrpc::Value makeValue(const common::Blob<N> &val) {
    return common::hex_lower_0x(val.data(), val.size());
  }

  inline jsonrpc::Value makeValue(const common::Buffer &val) {
    return common::hex_lower_0x(val.asVector().data(), val.asVector().size());
  }

  inline jsonrpc::Value makeValue(common::BufferView val) {
    return common::hex_lower_0x(val.data(), val.size());
  }

  inline jsonrpc::Value makeValue(const primitives::DigestItem &val) {
    auto result = scale::encode(val);
    if (result.has_value()) {
      return common::hex_lower_0x(result.value());
    }
    throw jsonrpc::InternalErrorFault("Unable to encode arguments.");
  }

  inline jsonrpc::Value makeValue(const primitives::Version &val) {
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
                     api_data.emplace_back(hex_lower_0x(api.first));
                     api_data.emplace_back(makeValue(api.second));
                     return api_data;
                   });

    data["apis"] = std::move(apis);
    return data;
  }

  inline jsonrpc::Value makeValue(const primitives::BlockHeader &val) {
    jStruct data;
    data["parentHash"] = makeValue(common::hex_lower_0x(val.parent_hash));
    std::stringstream stream;
    stream << "0x" << std::hex << val.number;
    data["number"] = makeValue(stream.str());
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

  inline jsonrpc::Value makeValue(const primitives::RpcMethods &v) {
    jStruct res;
    res["version"] = makeValue(v.version);
    res["methods"] = makeValue(v.methods);
    return res;
  }

  inline jsonrpc::Value makeValue(const primitives::BlockData &val) {
    jStruct block;
    block["extrinsics"] = makeValue(val.body);
    block["header"] = makeValue(val.header);

    jStruct data;
    data["block"] = std::move(block);
    data["justification"] = makeValue(val.justification);
    return data;
  }

  inline jsonrpc::Value makeValue(const primitives::RuntimeDispatchInfo &v) {
    jStruct res;
    res["weight"] = makeValue(v.weight);
    res["partialFee"] = makeValue(v.partial_fee);
    using Class = primitives::RuntimeDispatchInfo::DispatchClass;
    switch (v.dispatch_class) {
      case Class::Normal:
        res["class"] = "normal";
        break;
      case Class::Mandatory:
        res["class"] = "mandatory";
        break;
      case Class::Operational:
        res["class"] = "operational";
        break;
    }
    return res;
  }

  inline jsonrpc::Value makeValue(
      const primitives::events::ExtrinsicLifecycleEvent &event) {
    return visit_in_place(
        event.params,
        [&event](std::nullopt_t) -> jsonrpc::Value {
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
          jArray peers;
          peers.resize(params.peers.size());
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

  inline jsonrpc::Value makeValue(const primitives::Extrinsic &v) {
    return common::hex_lower_0x(scale::encode(v.data).value());
  }

}  // namespace kagome::api

#endif  // KAGOME_CORE_API_EXTRINSIC_RESPONSE_VALUE_CONVERTER_HPP
