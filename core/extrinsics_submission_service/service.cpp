/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "extrinsics_submission_service/service.hpp"

namespace kagome::service {
  ExtrinsicSubmissionService::ExtrinsicSubmissionService(
      std::shared_ptr<ExtrinsicSubmissionProxy> api_proxy)
      : api_proxy_(std::move(api_proxy)),
        on_request_([this](const std::string &data) { processData(data); }) {
    auto &dispatcher = server_.GetDispatcher();

    dispatcher.AddMethod("author_submitExtrinsic",
                         &ExtrinsicSubmissionProxy::submit_extrinsic,
                         *api_proxy_);

    dispatcher.AddMethod("author_pendingExtrinsics",
                         &ExtrinsicSubmissionProxy::pending_extrinsics,
                         *api_proxy_);
  }

  void ExtrinsicSubmissionService::processData(const std::string &data) {
    auto &&formatted_response = server_.HandleRequest(data);
    std::string response(formatted_response->GetData(),
                         formatted_response->GetSize());

    on_response_(response);
  }
}  // namespace kagome::service
