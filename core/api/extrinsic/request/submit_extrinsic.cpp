/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/extrinsic/request/submit_extrinsic.hpp"

#include "common/buffer.hpp"
#include "primitives/extrinsic.hpp"

namespace kagome::api {
  SubmitExtrinsicRequest SubmitExtrinsicRequest::fromParams(
      const jsonrpc::Request::Parameters &params) {
    if (params.size() != 1) {
      throw jsonrpc::InvalidParametersFault("incorrect number of arguments");
    }

    const auto &arg0 = params[0];
    if (!arg0.IsString()) {
      throw jsonrpc::InvalidParametersFault("invalid argument");
    }

    auto &&hexified_extrinsic = arg0.AsString();

    auto &&buffer = common::Buffer::fromHex(hexified_extrinsic);
    if (!buffer) {
      throw jsonrpc::Fault(buffer.error().message());
    }

    auto &&extrinsic = primitives::Extrinsic{std::move(buffer.value())};
    return SubmitExtrinsicRequest{std::move(extrinsic)};
  }
}  // namespace kagome::api
