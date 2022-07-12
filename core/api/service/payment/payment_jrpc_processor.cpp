/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/service/payment/payment_jrpc_processor.hpp"

#include <boost/assert.hpp>

#include "api/jrpc/jrpc_method.hpp"
#include "api/jrpc/jrpc_server.hpp"

#include "api/service/payment/requests/query_info.hpp"

namespace kagome::api::payment {

  PaymentJRpcProcessor::PaymentJRpcProcessor(std::shared_ptr<JRpcServer> server,
                                             std::shared_ptr<PaymentApi> api)
      : api_{std::move(api)}, server_{std::move(server)} {
    BOOST_ASSERT(api_ != nullptr);
    BOOST_ASSERT(server_ != nullptr);
  }

  template <typename Request>
  using Handler = Method<Request, PaymentApi>;

  void PaymentJRpcProcessor::registerHandlers() {
    server_->registerHandler("payment_queryInfo",
                             Handler<request::QueryInfo>(api_));
  }

}  // namespace kagome::api::payment
