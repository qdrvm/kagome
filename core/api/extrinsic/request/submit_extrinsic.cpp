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

#include "api/extrinsic/request/submit_extrinsic.hpp"

#include "common/hexutil.hpp"
#include "primitives/extrinsic.hpp"
#include "scale/scale.hpp"

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
