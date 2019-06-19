/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_EXTRINSICS_SUBMISSION_SERVICE_IMPL_JSON_TRANSPORT_IMPL_HPP
#define KAGOME_CORE_EXTRINSICS_SUBMISSION_SERVICE_IMPL_JSON_TRANSPORT_IMPL_HPP

#include <boost/asio/io_context.hpp>
#include "extrinsics_submission_service/json_transport.hpp"
#include "extrinsics_submission_service/service_configuration.hpp"

namespace kagome::service {
  class JsonTransportImpl : public JsonTransport {
   public:
    ~JsonTransportImpl() override = default;

    JsonTransportImpl(boost::asio::io_context &context,
                      ExtrinsicSubmissionServiceConfiguration config);

   private:
    /**
     * @brief processes response
     * @param response data to send
     */
    void processResponse(const std::string &response);
  };
}  // namespace kagome::service

#endif  // KAGOME_CORE_EXTRINSICS_SUBMISSION_SERVICE_IMPL_JSON_TRANSPORT_IMPL_HPP
