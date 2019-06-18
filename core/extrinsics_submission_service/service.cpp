/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "service.hpp"

#include "extrinsics_submission_service/transport.hpp"

namespace kagome::service {
  ExtrinsicSubmissionService::ExtrinsicSubmissionService(
      boost::asio::io_context &context, ExtrinsicSubmissionServiceConfig config,
      ExtrinsicSubmissionProxy &api_proxy,
      std::shared_ptr<JsonTransport> transport)
      : context_{context},
        config_{config},
        api_proxy_(api_proxy),
        transport_{transport_} {
    auto &dispatcher = server_.GetDispatcher();
    dispatcher.AddMethod("author_submitExtrinsic",
                         &ExtrinsicSubmissionProxy::submit_extrinsic,
                         api_proxy_);
    dispatcher.AddMethod("author_pendingExtrinsics",
                         &ExtrinsicSubmissionProxy::pending_extrinsics,
                         api_proxy_);
    //      dispatcher.AddMethod("author_removeExtrinsic",
    //      &ExtrinsicSubmissionProxy::remove_extrinsic, api_proxy_);

    transport_->dataReceived().connect(
        [this](const std::string &data) { processData(data); });
  }

  void ExtrinsicSubmissionService::processData(const std::string &data) {
    auto && formatted_response = server_.HandleRequest(data);
    std::string_view response(formatted_response->GetData(), formatted_response->GetSize());
    transport_->sendResponse(response);
  }
}  // namespace kagome::service
