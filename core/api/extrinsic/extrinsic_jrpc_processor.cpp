/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/extrinsic/extrinsic_jrpc_processor.hpp"

#include "api/extrinsic/impl/extrinsic_api_impl.hpp"
#include "api/extrinsic/request/submit_extrinsic.hpp"
#include "api/jrpc/value_converter.hpp"

namespace kagome::api {

  ExtrinsicJRPCProcessor::ExtrinsicJRPCProcessor(
      std::shared_ptr<ExtrinsicApi> api)
      : api_(std::move(api)) {}

  void ExtrinsicJRPCProcessor::registerHandlers() {
    // register all api methods
    registerHandler(
        "author_submitExtrinsic",
        [this](const jsonrpc::Request::Parameters &params) -> jsonrpc::Value {
          auto request = SubmitExtrinsicRequest::fromParams(params);

          std::lock_guard<std::mutex> lock(mutex_);

          auto &&res = api_->submitExtrinsic(request.extrinsic);
          if (!res) {
            throw jsonrpc::Fault(res.error().message());
          }
          return makeValue(res.value());
        });

    registerHandler(
        "author_pendingExtrinsics",
        [this](const jsonrpc::Request::Parameters &params) -> jsonrpc::Value {
          // method has no params

          std::lock_guard<std::mutex> lock(mutex_);

          auto &&res = api_->pendingExtrinsics();
          if (!res) {
            throw jsonrpc::Fault(res.error().message());
          }

          return makeValue(res.value());
        });
    // other methods to be registered as soon as implemented
  }

}  // namespace kagome::api
