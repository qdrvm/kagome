/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/extrinsic/request/submit_extrinsic.hpp"

#include "common/hexutil.hpp"
#include "primitives/extrinsic.hpp"
#include "scale/scale.hpp"

namespace kagome::api {
  SubmitExtrinsicRequest SubmitExtrinsicRequest::fromParams(
      const jsonrpc::Request::Parameters &params) {
    if (params.size() != 1) {
      throw jsonrpc::Fault("incorrect number of arguments");
    }

    const auto &arg0 = params[0];
    if (!arg0.IsString()) {
      throw jsonrpc::Fault("invalid argument");
    }

    auto &&hexified_extrinsic = arg0.AsString();

    auto &&buffer = common::unhexWith0x(hexified_extrinsic);
    if (!buffer) {
      throw jsonrpc::Fault(buffer.error().message());
    }

    auto &&extrinsic = scale::decode<primitives::Extrinsic>(buffer.value());
    if (!extrinsic) {
      throw jsonrpc::Fault(extrinsic.error().message());
    }
    return SubmitExtrinsicRequest{std::move(extrinsic.value())};
  }
}  // namespace kagome::api
