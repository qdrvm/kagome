/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/extrinsic/service.hpp"

#include "api/extrinsic/request/submit_extrinsic.hpp"
#include "api/extrinsic/response/value.hpp"
#include "api/transport/session.hpp"

namespace kagome::api {
  ExtrinsicApiService::ExtrinsicApiService(
      std::shared_ptr<server::Listener> listener,
      std::shared_ptr<ExtrinsicApi> api)
      : listener_{std::move(listener)}, api_(std::move(api)) {
    new_session_cnn_ = listener_->onNewSession().connect(
        [this](sptr<server::Session> session) {
          session->onRequest().connect([this](std::shared_ptr<server::Session> session, const std::string &request) {
            processData(session, request);
          });
        });

//    on_listener_stopped_cnn_ = listener_->onStopped().connect([](void(){
//        // TODO(yuraz): pre-230 log listener stop
//    }));

//    on_listener_error_cnn_ = listener_->onError().connect([](outcome::result<void> err) {
//      // TODO(yuraz): pre-230 process error
//    });

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

  void ExtrinsicApiService::processData(std::shared_ptr<server::Session> session, const std::string &data) {
    // TOOD(yuraz): pre-230 add mutex
    auto &&formatted_response = jsonrpc_handler_.HandleRequest(data);
    std::string response(formatted_response->GetData(),
                         formatted_response->GetSize());

    session->onResponse()(std::move(response));
  }

  void ExtrinsicApiService::start() {
    listener_->start();
  }

  void ExtrinsicApiService::stop() {
    new_session_cnn_.disconnect();
    listener_->stop();
  }
}  // namespace kagome::api
