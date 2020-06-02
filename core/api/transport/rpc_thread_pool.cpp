/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/transport/rpc_thread_pool.hpp"

namespace kagome::api {

  RpcThreadPool::RpcThreadPool(
      std::shared_ptr<AppStateManager> app_state_manager,
      std::shared_ptr<Context> context,
      const Configuration &configuration)
      : app_state_manager_(std::move(app_state_manager)),
        context_(std::move(context)),
        config_(configuration) {
    BOOST_ASSERT(app_state_manager_);
    BOOST_ASSERT(context_);

    app_state_manager_->atLaunch([this] { start(); });
    app_state_manager_->atShutdown([this] { stop(); });
  }

  void RpcThreadPool::start() {
    threads_.reserve(config_.max_thread_number);
    // Create a pool of threads to run all of the io_contexts.
    for (std::size_t i = 0; i < config_.min_thread_number; ++i) {
      auto thread = std::make_shared<std::thread>(
          [context = context_] { context->run(); });
      thread->detach();
      threads_.emplace_back(std::move(thread));
    }
  }

  void RpcThreadPool::stop() {
    context_->stop();
  }

}  // namespace kagome::api
