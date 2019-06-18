/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_SERVICE_EXTRINSICS_SUBMISSION_SERVICE_HPP
#define KAGOME_CORE_SERVICE_EXTRINSICS_SUBMISSION_SERVICE_HPP

#include <vector>

#include <jsonrpc-lean/server.h>
#include <boost/asio/io_context.hpp>

#include "extrinsics_submission_service/extrinsic_submission_proxy.hpp"

namespace kagome::service {
  class JsonTransport;

  struct ExtrinsicSubmissionServiceConfig {
    uint32_t port;
  };

  /**
   * @brief extrinsic submission service implementation
   */
  class ExtrinsicSubmissionService {
   public:
    /**
     * @brief constructor
     * @param context io_context reference
     * @param config server configuration
     * @param api_proxy extrinsic submission api proxy reference
     */
    ExtrinsicSubmissionService(boost::asio::io_context &context,
                               ExtrinsicSubmissionServiceConfig config,
                               ExtrinsicSubmissionProxy &api_proxy,
                               std::shared_ptr<JsonTransport> transport);

    /**
     * @brief handles decoded network message
     * @param data json request string
     */
    void processData(const std::string &data);

   private:
    boost::asio::io_context &context_;          ///< reference to io_context
    ExtrinsicSubmissionServiceConfig config_;   ///< server configuration
    jsonrpc::Server server_;                    ///< json rpc server instance
    ExtrinsicSubmissionProxy &api_proxy_;       ///< api reference
    std::shared_ptr<JsonTransport> transport_;  ///< json transport
  };

}  // namespace kagome::service

#endif  // KAGOME_CORE_SERVICE_EXTRINSICS_SUBMISSION_SERVICE_HPP
