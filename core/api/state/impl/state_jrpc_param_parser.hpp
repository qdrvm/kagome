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

#ifndef KAGOME_STATE_JRPC_PARAM_PARSER_HPP
#define KAGOME_STATE_JRPC_PARAM_PARSER_HPP

#include <boost/optional.hpp>
#include <tuple>

#include "api/jrpc/jrpc_processor.hpp"
#include "common/buffer.hpp"
#include "primitives/common.hpp"

namespace kagome::api {

  class StateJrpcParamParser {
   public:
    std::tuple<common::Buffer, boost::optional<primitives::BlockHash>>
    parseGetStorageParams(const jsonrpc::Request::Parameters &params) const;
  };

}  // namespace kagome::api

#endif  // KAGOME_STATE_JRPC_PARAM_PARSER_HPP
