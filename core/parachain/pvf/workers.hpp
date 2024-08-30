/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <filesystem>
#include <list>
#include <queue>

#include "parachain/pvf/pvf_worker_types.hpp"

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

namespace kagome {
  class ThreadPool;
  class Watchdog;
}  // namespace kagome

namespace kagome::parachain {
  struct ProcessAndPipes;

  class PvfWorkers : public std::enable_shared_from_this<PvfWorkers> {
   public:
    PvfWorkers(const application::AppConfiguration &app_config,
               std::shared_ptr<Watchdog> watchdog,
               std::shared_ptr<libp2p::basic::Scheduler> scheduler);

    using Cb = std::function<void(outcome::result<Buffer>)>;
    struct Job {
      RuntimeCodePath code_path;
      Buffer args;
      Cb cb;
    };
    void execute(Job &&job);

   private:
    struct Worker {
      std::shared_ptr<ProcessAndPipes> process;
      std::optional<RuntimeCodePath> code_path;
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

    std::shared_ptr<ThreadPool> thread_pool_;
    std::shared_ptr<boost::asio::io_context> io_context_;
    std::shared_ptr<libp2p::basic::Scheduler> scheduler_;
    std::filesystem::path exe_;
    size_t max_;
    std::chrono::milliseconds timeout_;
    PvfWorkerInputConfig worker_config_;
    std::list<Worker> free_;
    size_t used_ = 0;
    std::queue<Job> queue_;
  };
}  // namespace kagome::parachain
