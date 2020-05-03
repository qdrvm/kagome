/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "submit_extrinsic.hpp"

#include "common/hexutil.hpp"
#include "primitives/extrinsic.hpp"
#include "scale/scale.hpp"

namespace kagome::api::author::request {

  outcome::result<void> SubmitExtrinsic::init(
      const jsonrpc::Request::Parameters &params) {
    if (params.size() != 1) {
      throw jsonrpc::InvalidParametersFault("incorrect number of arguments");
    }

    const auto &arg0 = params[0];
    if (!arg0.IsString()) {
      throw jsonrpc::InvalidParametersFault("invalid argument");
    }

    auto &&hexified_extrinsic = arg0.AsString();
    OUTCOME_TRY(buffer, common::unhexWith0x(hexified_extrinsic));
    OUTCOME_TRY(extrinsic,
                scale::decode<primitives::Extrinsic>(buffer));

    extrinsic_ = std::move(extrinsic);

    return outcome::success();
  }

outcome::result<common::Hash256> SubmitExtrinsic::execute() {
	return api_->submitExtrinsic(extrinsic_);
}


}  // namespace kagome::api::author::request
