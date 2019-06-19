/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "extrinsics_submission_service/impl/service_builder.hpp"
#include "extrinsics_submission_service/impl/json_transport_impl.hpp"

namespace kagome::service {
  std::shared_ptr<ExtrinsicSubmissionService>
  ExtrinsicSubmissionServiceBuilder::build(
      ExtrinsicSubmissionServiceConfiguration configuration,
      boost::asio::io_context &context,
      sptr<runtime::TaggedTransactionQueue> ttq,
      sptr<transaction_pool::TransactionPool> pool,
      sptr<hash::Hasher> hasher) {
    auto transport = std::make_shared<JsonTransportImpl>(context, configuration);
    auto api = std::make_shared<ExtrinsicSubmissionApi>(ttq, pool, hasher);
    auto api_proxy = std::make_shared<ExtrinsicSubmissionProxy>(api);
    auto service = std::make_shared<ExtrinsicSubmissionService>(api_proxy);

    transport->dataReceived().connect(service->onRequest());
    service->onResponse().connect(transport->onResponse());

    return service;
  }

}  // namespace kagome::service
