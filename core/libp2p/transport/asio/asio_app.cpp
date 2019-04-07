/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "asio_app.hpp"

#include <iostream>

#include <boost/assert.hpp>

/// http://think-async.com/Asio/Recipes

namespace libp2p::transport::asio {

  AsioApp::AsioApp(int threads)
      : threads_(threads - 1), signals_(context_, SIGINT, SIGTERM) {
    if (threads <= 0) {
      threads = std::thread::hardware_concurrency();
    }

    // NOLINTNEXTLINE
    BOOST_ASSERT_MSG(
        threads > 0,
        "Something weird happened - AsioApp is created with 0 threads");

    // Register signal handlers so that the daemon may be shut down. You may
    // also want to register for other signals, such as SIGHUP to trigger a
    // re-read of a configuration file.
    signals_.async_wait([&](boost::system::error_code ec, int signo) {
      // TODO(@warchant): log ec, signo
      context_.stop();
    });
  }

  AsioApp::~AsioApp() {
    work_.reset();

    /**
     * This will stop the context processing loop. Any tasks
     * you add behind this point will not execute.
     */
    context_.stop();

    /**
     * Will wait till all the threads in the thread pool are finished with
     * their assigned tasks and 'join' them. Just assume the threads inside
     * the threadpool will be destroyed by this method.
     */
    for (auto &&thread : pool_) {
      if (thread.joinable()) {
        thread.join();
      }
    }
    pool_.clear();
  }

  boost::asio::io_context &AsioApp::context() {
    return context_;
  }

  void AsioApp::run() {
    // put work to context, so that run does not finish
    work_ = std::make_unique<WorkType>(context_.get_executor());

    for (int i = 0; i < threads_; i++) {
      pool_.emplace_back([&]() { context_.run(); });
    }

    context_.run();
  }

  void AsioApp::run_for(std::chrono::milliseconds ms) {
    for (int i = 0; i < threads_; i++) {
      pool_.emplace_back([&]() { context_.run_for(ms); });
    }

    context_.run_for(ms);
  }

}  // namespace libp2p::transport::asio
