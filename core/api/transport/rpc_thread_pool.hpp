/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_API_RPC_THREAD_POOL_HPP
#define KAGOME_CORE_API_RPC_THREAD_POOL_HPP

#include <boost/asio/io_service.hpp>
#include <boost/asio/signal_set.hpp>
#include <set>
#include <thread>

#include "api/transport/rpc_io_context.hpp"
#include "application/app_state_manager.hpp"
#include "log/logger.hpp"

using kagome::application::AppStateManager;

namespace kagome::api {

  /**
   * @brief thread pool for serve RPC calls
   */
  class RpcThreadPool : public std::enable_shared_from_this<RpcThreadPool> {
   public:
    using Context = RpcContext;

    struct Configuration {
      size_t min_thread_number = 1;
      size_t max_thread_number = 10;
    };

    RpcThreadPool(std::shared_ptr<Context> context,
                  const Configuration &configuration);

    ~RpcThreadPool() = default;

    /**
     * @brief starts pool
     */
    void start();

    /**
     * @brief stops pool
     */
    void stop();

   private:
    std::shared_ptr<Context> context_;
    const Configuration config_;

    std::vector<std::shared_ptr<std::thread>> threads_;

    log::Logger logger_ = log::createLogger("RpcThreadPool", "rpc_transport");
  };

}  // namespace kagome::api

#endif  // KAGOME_CORE_API_RPC_THREAD_POOL_HPP
