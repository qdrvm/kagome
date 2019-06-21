/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "extrinsics_submission_service/extrinsic_submission_proxy.hpp"

namespace kagome::service {
  ExtrinsicSubmissionProxy::ExtrinsicSubmissionProxy(
      sptr<ExtrinsicSubmissionApiImpl> api)
      : api_(std::move(api)) {}

  std::vector<uint8_t> ExtrinsicSubmissionProxy::submit_extrinsic(
      std::vector<uint8_t> bytes) {
    auto &&res = api_->submit_extrinsic(
        primitives::Extrinsic{common::Buffer(std::move(bytes))});
    if (!res) {
      throw jsonrpc::Fault(res.error().message());
    }
    auto &&value = res.value();
    return std::vector<uint8_t>(value.begin(), value.end());
  }

  std::vector<std::vector<uint8_t>>
  ExtrinsicSubmissionProxy::pending_extrinsics() {
    auto &&res = api_->pending_extrinsics();
    if (!res) {
      throw jsonrpc::Fault(res.error().message());
    }
    return res.value();
  }
}  // namespace kagome::service
