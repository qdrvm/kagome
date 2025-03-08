/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <deque>
#include <filesystem>
#include <list>
#include <mutex>

#include "metrics/metrics.hpp"
#include "parachain/pvf/pvf_worker_types.hpp"
#include "runtime/runtime_api/parachain_host_types.hpp"

namespace boost::asio {
  class io_context;
}  // namespace boost::asio

namespace libp2p::basic {
  class Scheduler;
}  // namespace libp2p::basic

namespace kagome {
  class PoolHandler;
}  // namespace kagome

namespace kagome::application {
  class AppConfiguration;
}  // namespace kagome::application

namespace kagome::common {
  class MainThreadPool;
}  // namespace kagome::common

namespace kagome::parachain {
  using runtime::PvfExecTimeoutKind;

  struct ProcessAndPipes;

  class PvfWorkers : public std::enable_shared_from_this<PvfWorkers> {
   public:
    PvfWorkers(const application::AppConfiguration &app_config,
               common::MainThreadPool &main_thread_pool,
               SecureModeSupport secure_mode_support,
               std::shared_ptr<libp2p::basic::Scheduler> scheduler);

    using Cb = std::function<void(outcome::result<Buffer>)>;
    struct Job {
      PvfWorkerInputCodeParams code_params;
      Buffer args;
      Cb cb;
      PvfExecTimeoutKind kind;
      std::chrono::milliseconds timeout{0};
    };
    void execute(Job &&job);

   private:
    struct Worker {
      std::shared_ptr<ProcessAndPipes> process;
      std::optional<PvfWorkerInputCodeParams> code_params;
    };
    struct Used {
      Used(PvfWorkers &self);
      Used(const Used &) = delete;
      void operator=(const Used &) = delete;
      ~Used();

      std::weak_ptr<PvfWorkers> weak_self;
    };

    void findFree(Job &&job);
    void writeCode(Job &&job, Worker &&worker, std::shared_ptr<Used> &&used);
    void call(Job &&job, Worker &&worker, std::shared_ptr<Used> &&used);
    void dequeue();

    std::shared_ptr<boost::asio::io_context> io_context_;
    std::shared_ptr<PoolHandler> main_pool_handler_;
    std::shared_ptr<libp2p::basic::Scheduler> scheduler_;
    std::filesystem::path exe_;
    size_t max_;
    PvfWorkerInputConfig worker_config_;
    std::list<Worker> free_;
    mutable std::mutex free_mutex_;  // Mutex for protecting free_ list
    size_t used_ = 0;
    std::unordered_map<PvfExecTimeoutKind, std::deque<Job>> queues_;

    metrics::RegistryPtr metrics_registry_ = metrics::createRegistry();
    std::unordered_map<PvfExecTimeoutKind, metrics::Gauge *> metric_queue_size_;
  };
}  // namespace kagome::parachain
