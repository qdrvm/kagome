/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "extrinsics_submission_service/extrinsic_submission_proxy.hpp"

#include <jsonrpc-lean/value.h>

namespace kagome::service {
  using primitives::Extrinsic;

  ExtrinsicSubmissionProxy::ExtrinsicSubmissionProxy(
      sptr<ExtrinsicSubmissionApi> api)
      : api_(std::move(api)) {}

  std::vector<uint8_t> ExtrinsicSubmissionProxy::submit_extrinsic(
      const std::string &hexified_extrinsic) {
    // hex-decode extrinsic data
    auto &&buffer = common::Buffer::fromHex(hexified_extrinsic);
    if (!buffer) {
      throw jsonrpc::Fault(buffer.error().message());
    }

    auto &&extrinsic = Extrinsic{std::move(buffer.value())};
    auto &&res = api_->submit_extrinsic(std::move(extrinsic));
    if (!res) {
      throw jsonrpc::Fault(res.error().message());
    }
    auto &&value = res.value();
    return std::vector<uint8_t>(value.begin(), value.end());
  }

  //  std::vector<std::vector<uint8_t>>
  //  ExtrinsicSubmissionProxy::pending_extrinsics() {
  //    auto &&res = api_->pending_extrinsics();
  //    if (!res) {
  //      throw jsonrpc::Fault(res.error().message());
  //    }
  //    auto &&extrinsics = res.value();
  //    std::vector<std::vector<uint8_t>> values;
  //    values.reserve(extrinsics.size());
  //    for (auto & e: extrinsics) {
  //      values.push_back(e.data.toVector());
  //    }
  //    return values;
  //  }
}  // namespace kagome::service
