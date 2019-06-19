/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_EXTRINSICS_SUBMISSION_SERVICE_SERVICE_BUILDER_HPP
#define KAGOME_CORE_EXTRINSICS_SUBMISSION_SERVICE_SERVICE_BUILDER_HPP

#include <boost/asio/io_context.hpp>

#include "extrinsics_submission_service/extrinsic_submission_proxy.hpp"
#include "extrinsics_submission_service/json_transport.hpp"
#include "extrinsics_submission_service/service.hpp"
#include "extrinsics_submission_service/service_configuration.hpp"

namespace kagome::service {
  class ExtrinsicSubmissionServiceBuilder {
    template <class T>
    using sptr = std::shared_ptr<T>;

   public:
    std::shared_ptr<ExtrinsicSubmissionService> build(
        ExtrinsicSubmissionServiceConfiguration configuration,
        boost::asio::io_context &context,
        std::shared_ptr<runtime::TaggedTransactionQueue> ttq,
        std::shared_ptr<transaction_pool::TransactionPool> pool,
        std::shared_ptr<hash::Hasher> hasher);
  };
}  // namespace kagome::service

#endif  // KAGOME_CORE_EXTRINSICS_SUBMISSION_SERVICE_SERVICE_BUILDER_HPP
