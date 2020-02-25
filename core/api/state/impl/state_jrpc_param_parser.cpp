/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/state/impl/state_jrpc_param_parser.hpp"

namespace kagome::api {

  std::tuple<common::Buffer, boost::optional<primitives::BlockHash>>
  StateJrpcParamParser::parseGetStorageParams(
      const jsonrpc::Request::Parameters &params) const {
    if (params.size() > 2 or params.empty()) {
      throw jsonrpc::InvalidParametersFault("Incorrect number of params");
    }
    auto& param0 = params[0];
    if(not param0.IsString()) {
      throw jsonrpc::InvalidParametersFault("Parameter 'key' must be a hex string");
    }
    auto&& key_str = param0.AsString();
    auto&& key = common::Buffer::fromHex(key_str);
    if(not key) {
      throw jsonrpc::Fault(key.error().message());
    }
    if(params.size() > 1) {
      auto& param1 = params[1];
      if(not param0.IsString()) {
        throw jsonrpc::InvalidParametersFault("Parameter 'at' must be a hex string");
      }
      auto&& at_str = param1.AsString();
      auto&& at = primitives::BlockHash::fromHex(at_str);
      if(not at) {
        throw jsonrpc::Fault(at.error().message());
      }
      return std::make_tuple(std::move(key.value()), boost::make_optional(at.value()));
    }
    return std::make_tuple(std::move(key.value()), boost::none);
  }

}  // namespace kagome::api
