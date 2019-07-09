/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_SERVICE_EXTRINSICS_SUBMISSION_SERVICE_HPP
#define KAGOME_CORE_SERVICE_EXTRINSICS_SUBMISSION_SERVICE_HPP

#include <vector>

#include <jsonrpc-lean/server.h>
#include <boost/core/noncopyable.hpp>
#include <boost/signals2/signal.hpp>
#include "api/extrinsic/extrinsic_api.hpp"
#include "api/transport/impl/listener_impl.hpp"

namespace kagome::api {

  /**
   * @brief extrinsic submission service implementation
   */
  class ExtrinsicApiService : private boost::noncopyable {
    using SignalType = void(const std::string &);
    template <class T>
    using sptr = std::shared_ptr<T>;

    template <class T>
    using Signal = boost::signals2::signal<T>;

    using Connection = boost::signals2::connection;

   public:
    /**
     * @brief constructor
     * @param context io_context reference
     * @param config server configuration
     * @param api_proxy extrinsic submission api proxy reference
     */
    ExtrinsicApiService(std::shared_ptr<server::Listener> listener,
                        std::shared_ptr<ExtrinsicApi> api);

    /**
     * @brief starts service
     * @return true if successful, false otherwise
     */
    void start();

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
    void processData(const std::shared_ptr<server::Session> &session,
                     const std::string &data);

    jsonrpc::JsonFormatHandler format_handler_{};  ///< format handler instance
    jsonrpc::Server jsonrpc_handler_{};            ///< json rpc server instance
    sptr<server::Listener> listener_;              ///< endpoint listener
    sptr<ExtrinsicApi> api_;                       ///< api implementation
  };

}  // namespace kagome::api

#endif  // KAGOME_CORE_SERVICE_EXTRINSICS_SUBMISSION_SERVICE_HPP
