/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_SERVICE_EXTRINSICS_SUBMISSION_SERVICE_HPP
#define KAGOME_CORE_SERVICE_EXTRINSICS_SUBMISSION_SERVICE_HPP

#include <vector>

#include <jsonrpc-lean/server.h>
#include <boost/signals2/signal.hpp>
#include "api/extrinsic/extrinsic_api.hpp"
#include "api/transport/basic_transport.hpp"

namespace kagome::api {

  /**
   * @brief extrinsic submission service implementation
   */
  class ExtrinsicApiService {
    using SignalType = void(const std::string &);
    template <class T>
    using sptr = std::shared_ptr<T>;

    template <class T>
    using signal_t = boost::signals2::signal<T>;

    using connection_t = boost::signals2::connection;

   public:
    /**
     * @brief service configuration
     */
    struct Configuration;
    /**
     * @brief constructor
     * @param context io_context reference
     * @param config server configuration
     * @param api_proxy extrinsic submission api proxy reference
     */
    ExtrinsicApiService(std::shared_ptr<BasicTransport> transport,
                        std::shared_ptr<ExtrinsicApi> api);

    /**
     * @brief starts service
     * @return true if successful, false otherwise
     */
    outcome::result<void> start();

    /**
     * @brief stops listening
     */
    void stop();

   private:
    using Method = jsonrpc::MethodWrapper::Method;
    /**
     * @brief registers rpc request handler lambda
     * @param name rpc method name
     * @param method handler functor
     */
    void registerHandler(const std::string &name, Method method);
    /**
     * @brief handles decoded network message
     * @param data json request string
     */
    void processData(std::string_view data);

    jsonrpc::JsonFormatHandler format_handler_{};  ///< format handler instance
    jsonrpc::Server server_{};                     ///< json rpc server instance
    sptr<BasicTransport> transport_;               ///< json transport
    sptr<ExtrinsicApi> api_;                       ///< extrinsic api
    signal_t<SignalType> on_response_{};           ///< notifies response
    connection_t request_cnn_;   ///< request connection holder
    connection_t response_cnn_;  ///< response connection holder
  };

}  // namespace kagome::api

#endif  // KAGOME_CORE_SERVICE_EXTRINSICS_SUBMISSION_SERVICE_HPP
