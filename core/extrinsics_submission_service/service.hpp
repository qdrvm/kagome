/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_SERVICE_EXTRINSICS_SUBMISSION_SERVICE_HPP
#define KAGOME_CORE_SERVICE_EXTRINSICS_SUBMISSION_SERVICE_HPP

#include <vector>

#include <jsonrpc-lean/server.h>
#include <boost/asio/io_context.hpp>
#include <boost/signals2/signal.hpp>

#include "extrinsics_submission_service/extrinsic_submission_proxy.hpp"

namespace kagome::service {

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
    explicit ExtrinsicSubmissionService(ExtrinsicSubmissionProxy &api_proxy);

    /**
     * @return request handler slot
     */
    inline auto &onRequest() {
      return on_request_;
    }

    /**
     * @return response signal
     */
    inline auto &onResponse() {
      return on_response_;
    }

   private:
    /**
     * @brief handles decoded network message
     * @param data json request string
     */
    void processData(const std::string &data);

    jsonrpc::Server server_;               ///< json rpc server instance
    ExtrinsicSubmissionProxy &api_proxy_;  ///< api reference

    boost::signals2::slot<void(const std::string &)>
        on_request_;  ///< received data handler

    boost::signals2::signal<void(const std::string &)>
        on_response_;  ///< notifies response
  };

}  // namespace kagome::service

#endif  // KAGOME_CORE_SERVICE_EXTRINSICS_SUBMISSION_SERVICE_HPP
