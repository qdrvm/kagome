/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/extrinsic/service.hpp"

#include "api/extrinsic/request/submit_extrinsic.hpp"
#include "api/extrinsic/response/value.hpp"

namespace kagome::api {
  ExtrinsicApiService::ExtrinsicApiService(
      std::shared_ptr<server::Listener> listener,
      std::shared_ptr<ExtrinsicApi> api)
      : listener_{std::move(listener)}, api_(std::move(api)) {
    //    request_cnn_ =
    new_session_cnn_ = listener_->onNewSession().connect(
        [this](sptr<server::Session> session) {
          session->connect(static_cast<WorkerApi&>(*this));
        });

//    transport_->dataReceived().connect(
//        [this](const std::string &data) { processData(data); });
//
//    //    response_cnn_ =
//    on_response_.connect(transport_->onResponse());

    // register json format handler
    jsonrpc_handler_.RegisterFormatHandler(format_handler_);

    // register all api methods
    registerHandler("author_submitExtrinsic",
                    [s = api_](const jsonrpc::Request::Parameters &params)
                        -> jsonrpc::Value {
                      auto request = SubmitExtrinsicRequest::fromParams(params);

                      auto &&res = s->submitExtrinsic(request.extrinsic);
                      if (!res) {
                        throw jsonrpc::Fault(res.error().message());
                      }
                      return makeValue(res.value());
                    });

    registerHandler("author_pendingExtrinsics",
                    [s = api_](const jsonrpc::Request::Parameters &params)
                        -> jsonrpc::Value {
                      auto &&res = s->pendingExtrinsics();
                      if (!res) {
                        throw jsonrpc::Fault(res.error().message());
                      }

                      return makeValue(res.value());
                    });
    // other methods to be registered as soon as implemented
  }

  ExtrinsicApiService::~ExtrinsicApiService() {
    new_session_cnn_.disconnect();
  }

  void ExtrinsicApiService::registerHandler(const std::string &name,
                                            Method method) {
    auto &dispatcher = jsonrpc_handler_.GetDispatcher();
    dispatcher.AddMethod(name, std::move(method));
  }

  void ExtrinsicApiService::processData(const std::string &data) {
    auto &&formatted_response = jsonrpc_handler_.HandleRequest(data);
    std::string response(formatted_response->GetData(),
                         formatted_response->GetSize());

    on_response_(response);
  }

  outcome::result<void> ExtrinsicApiService::start() {
    listener_->start();
    return outcome::success();
  }

  void ExtrinsicApiService::stop() {
    listener_->stop();
  }
}  // namespace kagome::api
