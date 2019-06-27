/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/extrinsic/service.hpp"

#include "api/extrinsic/request/submit_extrinsic.hpp"
#include "api/extrinsic/response/value.hpp"

namespace kagome::api {
  ExtrinsicApiService::ExtrinsicApiService(sptr<BasicTransport> transport,
                                           sptr<ExtrinsicApi> api)
      : transport_{std::move(transport)}, api_(std::move(api)) {
    request_cnn_ = transport_->dataReceived().connect(
        [this](std::string_view data) { processData(data); });

    response_cnn_ = on_response_.connect(transport_->onResponse());

    // register json format handler
    server_.RegisterFormatHandler(format_handler_);

    // register all api methods
    registerHandler(
        "author_submitExtrinsic",
        [s = api_](const jsonrpc::Request::Parameters &params) -> jsonrpc::Value {
          auto request = SubmitExtrinsicRequest::fromParams(params);

          auto &&res = s->submitExtrinsic(request.extrinsic);
          if (!res) {
            throw jsonrpc::Fault(res.error().message());
          }
          return makeValue(res.value());
        });

    registerHandler(
        "author_pendingExtrinsics",
        [s = api_](const jsonrpc::Request::Parameters &params) -> jsonrpc::Value {
          auto &&res = s->pendingExtrinsics();
          if (!res) {
            throw jsonrpc::Fault(res.error().message());
          }

          return makeValue(res.value());
        });
    // other methods to be registered as soon as implemented
  }

  void ExtrinsicApiService::registerHandler(const std::string &name,
                                            Method method) {
    auto &dispatcher = server_.GetDispatcher();
    dispatcher.AddMethod(name, std::move(method));
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
