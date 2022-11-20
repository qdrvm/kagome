/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */
#include "parachain/thread_pool.hpp"

namespace kagome::thread {

  ThreadPool::ThreadPool(
      std::shared_ptr<application::AppStateManager> app_state_manager,
      size_t thread_count)
      : thread_count_{thread_count} {
    BOOST_ASSERT(thread_count_ > 0);

    if (app_state_manager) app_state_manager->takeControl(*this);
  }

  ThreadPool::ThreadPool(size_t thread_count) : thread_count_{thread_count} {
    BOOST_ASSERT(thread_count_ > 0);
  }

  ThreadPool::~ThreadPool() {
    /// check that all workers are stopped.
    BOOST_ASSERT(workers_.empty());
  }

  bool ThreadPool::prepare() {
    context_ = std::make_shared<WorkersContext>();
    work_guard_ = std::make_unique<WorkGuard>(context_->get_executor());
    workers_.reserve(thread_count_);
    return true;
  }

  bool ThreadPool::start() {
    BOOST_ASSERT(context_);
    BOOST_ASSERT(work_guard_);
    for (size_t ix = 0; ix < thread_count_; ++ix) {
      workers_.emplace_back(
          [wptr{this->weak_from_this()}, context{context_}]() {
            if (auto self = wptr.lock()) {
              self->logger_->debug("Started thread worker with id: {}",
                                   std::this_thread::get_id());
            }
            context->run();
          });
    }
    return true;
  }

  void ThreadPool::stop() {
    work_guard_.reset();
    if (context_) {
      context_->stop();
    }
    for (auto &worker : workers_) {
      if (worker.joinable()) {
        worker.join();
      }
    }
    workers_.clear();
  }

}  // namespace kagome::thread
