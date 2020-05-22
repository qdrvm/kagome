/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/transport/rpc_thread_pool.hpp"

namespace kagome::api {

  RpcThreadPool::RpcThreadPool(std::shared_ptr<Context> context,
                               const Configuration &configuration)
      : context_(std::move(context)), config_(configuration) {
    BOOST_ASSERT(context_);
    signals_ = std::make_unique<boost::asio::signal_set>(*context_);

    // Register to handle the signals that indicate when the server should exit.
    // It is safe to register for the same signal multiple times in a program,
    // provided all registration for the specified signal is made through Asio.
    signals_->add(SIGINT);
    signals_->add(SIGTERM);
#if defined(SIGQUIT)
    signals_->add(SIGQUIT);
#endif  // defined(SIGQUIT)
    signals_->async_wait(std::bind(&RpcThreadPool::stop, this));
  }

  void RpcThreadPool::start() {
    // Create a pool of threads to run all of the io_contexts.
    for (std::size_t i = 0; i < config_.min_thread_number; ++i) {
      auto thread = std::make_shared<std::thread>(
          [context = context_] { context->run(); });
      thread->detach();
      threads_.emplace(std::move(thread));
    }
  }

  void RpcThreadPool::stop() {
    context_->stop();
  }

}  // namespace kagome::api
