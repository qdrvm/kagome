/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "extrinsics_submission_service/service.hpp"
#include "extrinsics_submission_service/impl/json_transport_impl.hpp"

namespace kagome::service {
  ExtrinsicSubmissionService::ExtrinsicSubmissionService(
      Configuration configuration, sptr<JsonTransport> transport,
      sptr<ExtrinsicSubmissionApi> api)
      : configuration_{configuration},
        transport_{transport},
        api_proxy_(std::make_shared<ExtrinsicSubmissionProxy>(api)),
        on_request_([this](std::string_view data) { processData(data); }) {
    api_proxy_ = std::make_shared<ExtrinsicSubmissionProxy>(api);

    transport->dataReceived().connect(on_request_);
    on_response_.connect(transport->onResponse());

    // register json format handler
    server_.RegisterFormatHandler(json_format_handler_);

    auto &dispatcher = server_.GetDispatcher();

    // register all api methods
    dispatcher.AddMethod("author_submitExtrinsic",
                         &ExtrinsicSubmissionProxy::submit_extrinsic,
                         *api_proxy_);

    dispatcher.AddMethod("author_pendingExtrinsics",
                         &ExtrinsicSubmissionProxy::pending_extrinsics,
                         *api_proxy_);
    // other methods to be registered as soon as implemented
  }

  void ExtrinsicSubmissionService::processData(std::string_view data) {
    auto &&formatted_response = server_.HandleRequest(std::string(data));
    std::string response(formatted_response->GetData(),
                         formatted_response->GetSize());

    on_response_(response);
  }

  bool ExtrinsicSubmissionService::start() {
    return transport_->start(configuration_.port);
  }

  void ExtrinsicSubmissionService::stop() {
    transport_->stop();
  }
}  // namespace kagome::service
