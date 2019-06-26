/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/extrinsic/service.hpp"

namespace kagome::api {
  ExtrinsicApiService::ExtrinsicApiService(
      Configuration configuration, sptr<BasicTransport> transport,
      sptr<ExtrinsicApi> api)
      : configuration_{configuration},
        transport_{std::move(transport)},
        api_proxy_(std::make_shared<ExtrinsicApiProxy>(std::move(api))) {
    request_cnn_ = transport_->dataReceived().connect(
        [this](std::string_view data) { processData(data); });

    response_cnn_ = on_response_.connect(transport_->onResponse());

    // register json format handler
    server_.RegisterFormatHandler(format_handler_);

    auto &dispatcher = server_.GetDispatcher();

    // register all api methods
    dispatcher.AddMethod("author_submitExtrinsic",
                         &ExtrinsicApiProxy::submit_extrinsic, *api_proxy_);

    dispatcher.AddMethod("author_pendingExtrinsics",
                         &ExtrinsicApiProxy::pending_extrinsics, *api_proxy_);
    // other methods to be registered as soon as implemented
  }

  void ExtrinsicApiService::processData(std::string_view data) {
    auto &&formatted_response = server_.HandleRequest(std::string(data));
    std::string response(formatted_response->GetData(),
                         formatted_response->GetSize());

    on_response_(response);
  }

  outcome::result<void> ExtrinsicApiService::start() {
    return transport_->start();
  }

  void ExtrinsicApiService::stop() {
    transport_->stop();
  }
}  // namespace kagome::api
