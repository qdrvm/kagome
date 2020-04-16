/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 *
 * Kagome is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * Kagome is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
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
