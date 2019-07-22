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
    listener_->onError().connect([this](outcome::result<void> err) { stop(); });

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

  void ExtrinsicApiService::registerHandler(const std::string &name,
                                            Method method) {
    auto &dispatcher = jsonrpc_handler_.GetDispatcher();
    dispatcher.AddMethod(name, std::move(method));
  }

  void ExtrinsicApiService::processData(const sptr<server::Session> &session,
                                        const std::string &data) {
    auto &&formatted_response = jsonrpc_handler_.HandleRequest(data);
    std::string response(formatted_response->GetData(),
                         formatted_response->GetSize());

    session->onResponse()(std::move(response));
  }

  void ExtrinsicApiService::start() {
    listener_->start(
        // NOLINTNEXTLINE(performance-unnecessary-value-param)
        [this](sptr<server::Session> session) {
          session->onRequest().connect(
              // NOLINTNEXTLINE(performance-unnecessary-value-param)
              [this](std::shared_ptr<server::Session> session,
                     const std::string &request) {
                processData(session, request);
              });
        });
  }

  void ExtrinsicApiService::stop() {
    listener_->stop();
  }
}  // namespace kagome::api
