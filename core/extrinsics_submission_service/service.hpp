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
#include "extrinsics_submission_service/json_transport.hpp"

namespace kagome::service {

  /**
   * @brief extrinsic submission service implementation
   */
  class ExtrinsicSubmissionService {
    using SignalType = void(std::string_view);
    template <class T>
    using sptr = std::shared_ptr<T>;

   public:
    /**
     * @brief service configuration
     */
    struct Configuration {
      uint32_t port;  ///< port to listen
    };

    /**
     * @brief constructor
     * @param context io_context reference
     * @param config server configuration
     * @param api_proxy extrinsic submission api proxy reference
     */
    ExtrinsicSubmissionService(Configuration configuration,
                               std::shared_ptr<JsonTransport> transport,
                               std::shared_ptr<ExtrinsicSubmissionApi> api);

    /**
     * @brief starts service
     * @return true if successful, false otherwise
     */
    bool start();

    /**
     * @brief stops listening
     */
    void stop();

   private:
    /**
     * @brief handles decoded network message
     * @param data json request string
     */
    void processData(std::string_view data);

    jsonrpc::JsonFormatHandler
        json_format_handler_{};                 ///< format handler instance
    jsonrpc::Server server_{};                  ///< json rpc server instance
    Configuration configuration_;               ///< service configuration
    sptr<JsonTransport> transport_;             ///< json transport
    sptr<ExtrinsicSubmissionProxy> api_proxy_;  ///< api reference
    boost::signals2::slot<SignalType> on_request_;  ///< received data handler
    boost::signals2::signal<SignalType> on_response_{};  ///< notifies response
  };

}  // namespace kagome::service

#endif  // KAGOME_CORE_SERVICE_EXTRINSICS_SUBMISSION_SERVICE_HPP
