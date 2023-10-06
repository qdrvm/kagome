/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_PAYMENT_JRPC_PROCESSOR_HPP
#define KAGOME_PAYMENT_JRPC_PROCESSOR_HPP

#include "api/jrpc/jrpc_processor.hpp"

#include <memory>

namespace kagome::api {
  class JRpcServer;
  class PaymentApi;
}  // namespace kagome::api

namespace kagome::api::payment {

  class PaymentJRpcProcessor : public JRpcProcessor {
   public:
    PaymentJRpcProcessor(std::shared_ptr<JRpcServer> server,
                         std::shared_ptr<PaymentApi> api);

   private:
    void registerHandlers() override;

    std::shared_ptr<PaymentApi> api_;
    std::shared_ptr<JRpcServer> server_;
  };

}  // namespace kagome::api::payment

#endif  // KAGOME_PAYMENT_JRPC_PROCESSOR_HPP
