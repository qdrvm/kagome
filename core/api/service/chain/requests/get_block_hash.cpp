/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/service/chain/requests/get_block_hash.hpp"

#include "common/hexutil.hpp"
#include "common/visitor.hpp"

namespace kagome::api::chain::request {
  using primitives::BlockHash;
  using primitives::BlockNumber;

  outcome::result<void> GetBlockhash::init(
      const jsonrpc::Request::Parameters &params) {
    if (params.empty()) {
      param_ = NoParameters{};
      return outcome::success();
    }

    if (params.size() > 1) {
      throw jsonrpc::InvalidParametersFault("incorrect number of arguments");
    }

    const auto &arg0 = params[0];
    if (arg0.IsInteger32()) {
      param_ = static_cast<uint32_t>(arg0.AsInteger32());
    } else if (arg0.IsString()) {
      param_ = arg0.AsString();
    } else if (arg0.IsArray()) {
      const auto &array = arg0.AsArray();
      // empty array would cause problems within `execute`
      if (array.empty()) {
        throw jsonrpc::InvalidParametersFault("invalid argument");
      }
      std::vector<VectorParam> param;
      param.reserve(array.size());
      for (const auto &v : array) {
        if (v.IsInteger32()) {
          param.emplace_back(VectorParam(v.AsInteger32()));
        } else if (v.IsString()) {
          param.emplace_back(VectorParam(v.AsString()));
        } else {
          throw jsonrpc::InvalidParametersFault("invalid argument");
        }
      }
      param_ = std::move(param);
    } else {
      throw jsonrpc::InvalidParametersFault("invalid argument");
    }

    return outcome::success();
  }  // namespace kagome::api::chain::request

  namespace {
    std::string formatBlockHash(const BlockHash &bh) {
      static const std::string prefix = "0x";
      auto hex = common::hex_lower(bh);
      return prefix + hex;
    }
  }  // namespace

  outcome::result<GetBlockhash::ResultType> GetBlockhash::execute() {
    return kagome::visit_in_place(
        param_,
        [this](const NoParameters &v) -> outcome::result<ResultType> {
          // return last finalized
          OUTCOME_TRY(bh, api_->getBlockHash());
          return formatBlockHash(bh);
        },
        [this](BlockNumber v) -> outcome::result<ResultType> {
          OUTCOME_TRY(bh, api_->getBlockHash(v));
          return formatBlockHash(bh);
        },
        [this](std::string_view v) -> outcome::result<ResultType> {
          OUTCOME_TRY(bh, api_->getBlockHash(v));
          return formatBlockHash(bh);
        },
        [this](
            const std::vector<VectorParam> &v) -> outcome::result<ResultType> {
          OUTCOME_TRY(rr, api_->getBlockHash(v));
          std::vector<std::string> results{};
          results.reserve(v.size());
          for (const auto &it : rr) {
            results.emplace_back(formatBlockHash(it));
          }

          return results;
        });
  };

}  // namespace kagome::api::chain::request
