/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "parachain/pvf/workers.hpp"

#include <libp2p/basic/scheduler.hpp>
#include <qtils/option_take.hpp>

#include "application/app_configuration.hpp"
#include "common/main_thread_pool.hpp"
#include "parachain/pvf/pvf_worker_types.hpp"
#include "utils/get_exe_path.hpp"
#include "utils/process.hpp"

namespace kagome::parachain {
  PvfWorkers::PvfWorkers(const application::AppConfiguration &app_config,
                         common::MainThreadPool &main_thread_pool,
                         std::shared_ptr<libp2p::basic::Scheduler> scheduler)
      : io_context_{main_thread_pool.io_context()},
        main_pool_handler_{main_thread_pool.handlerStarted()},
        scheduler_{std::move(scheduler)},
        exe_{exePath()},
        max_{app_config.pvfMaxWorkers()},
        timeout_{app_config.pvfSubprocessDeadline()},
        worker_config_{
            pvf_runtime_engine(app_config),
            app_config.runtimeCacheDirPath(),
            app_config.log(),
            app_config.disableSecureMode(),
        } {}

  void PvfWorkers::execute(Job &&job) {
    REINVOKE(*main_pool_handler_, execute, std::move(job));
    if (free_.empty()) {
      if (used_ >= max_) {
        queue_.emplace(std::move(job));
        return;
      }
      auto used = std::make_shared<Used>(*this);
      auto process = ProcessAndPipes::make(*io_context_, exe_, {"pvf-worker"});
      process->writeScale(
          worker_config_,
          [WEAK_SELF, job{std::move(job)}, used{std::move(used)}, process](
              outcome::result<void> r) mutable {
            WEAK_LOCK(self);
            if (not r) {
              return job.cb(r.error());
            }
            self->writeCode(
                std::move(job),
                {.process = std::move(process), .code_path = std::nullopt},
                std::move(used));
          });
      return;
    }
    findFree(std::move(job));
  }

  void PvfWorkers::findFree(Job &&job) {
    auto it =
        std::find_if(free_.begin(), free_.end(), [&](const Worker &worker) {
          return worker.code_path == job.code_path;
        });
    if (it == free_.end()) {
      it = free_.begin();
    }
    auto worker = std::move(*it);
    free_.erase(it);
    writeCode(std::move(job), std::move(worker), std::make_shared<Used>(*this));
  }

  PvfWorkers::Used::Used(PvfWorkers &self) : weak_self{self.weak_from_this()} {
    ++self.used_;
  }

  PvfWorkers::Used::~Used() {
    IF_WEAK_LOCK(self) {
      --self->used_;
    }
  }

  void PvfWorkers::writeCode(Job &&job,
                             Worker &&worker,
                             std::shared_ptr<Used> &&used) {
    if (worker.code_path == job.code_path) {
      call(std::move(job), std::move(worker), std::move(used));
      return;
    }
    worker.code_path = job.code_path;
    auto code_path = PvfWorkerInput{job.code_path};

    worker.process->writeScale(
        code_path,
        [WEAK_SELF, job{std::move(job)}, worker, used{std::move(used)}](
            outcome::result<void> r) mutable {
          WEAK_LOCK(self);
          if (not r) {
            return job.cb(r.error());
          }
          self->call(std::move(job), std::move(worker), std::move(used));
        });
  }

  void PvfWorkers::call(Job &&job,
                        Worker &&worker,
                        std::shared_ptr<Used> &&used) {
    auto timeout = std::make_shared<libp2p::Cancel>();
    auto cb_shared = std::make_shared<std::optional<Cb>>(
        [WEAK_SELF, cb{std::move(job.cb)}, worker, used{std::move(used)}](
            outcome::result<Buffer> r) mutable {
          WEAK_LOCK(self);
          cb(std::move(r));
          if (not r) {
            return;
          }
          self->free_.emplace_back(std::move(worker));
          self->dequeue();
        });
    auto cb = [cb_shared, timeout](outcome::result<Buffer> r) mutable {
      if (auto cb = qtils::optionTake(*cb_shared)) {
        (*cb)(std::move(r));
      }
      timeout->reset();
    };
    *timeout = scheduler_->scheduleWithHandle(
        [cb]() mutable { cb(std::errc::timed_out); }, timeout_);
    worker.process->writeScale(PvfWorkerInput{job.args},
                               [cb](outcome::result<void> r) mutable {
                                 if (not r) {
                                   return cb(r.error());
                                 }
                               });
    worker.process->read(std::move(cb));
  }

  void PvfWorkers::dequeue() {
    if (queue_.empty()) {
      return;
    }
    auto job = std::move(queue_.front());
    queue_.pop();
    findFree(std::move(job));
  }
}  // namespace kagome::parachain
