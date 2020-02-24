/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
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
