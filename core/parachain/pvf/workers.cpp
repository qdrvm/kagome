/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "parachain/pvf/workers.hpp"

#include <boost/asio/local/stream_protocol.hpp>
#include <boost/process.hpp>
#include <libp2p/basic/scheduler.hpp>
#include <libp2p/common/asio_buffer.hpp>
#include <qtils/option_take.hpp>

#include "application/app_configuration.hpp"
#include "common/main_thread_pool.hpp"
#include "filesystem/common.hpp"
#include "macro/feature_macros.hpp"
#include "parachain/pvf/pvf_worker_types.hpp"
#include "utils/get_exe_path.hpp"
#include "utils/weak_macro.hpp"

namespace kagome::parachain {
  using unix = boost::asio::local::stream_protocol;

  constexpr auto kMetricQueueSize = "kagome_pvf_queue_size";

  struct ProcessAndPipes : std::enable_shared_from_this<ProcessAndPipes> {
    boost::process::child process;
    std::optional<unix::socket> socket;
    std::shared_ptr<Buffer> writing = std::make_shared<Buffer>();
    std::shared_ptr<Buffer> reading = std::make_shared<Buffer>();

    struct Config {
      bool disable_lsan = false;
    };

    ProcessAndPipes(boost::asio::io_context &io_context,
                    const std::string &exe,
                    const std::string &unix_socket_path,
                    const Config &config)
        : process{
              exe,
              boost::process::args({"pvf-worker", unix_socket_path}),
              boost::process::env(boost::process::environment()),
// LSAN doesn't work in secure mode
#if KAGOME_WITH_ASAN
              boost::process::env["ASAN_OPTIONS"] =
                  config.disable_lsan ? "detect_leaks=0" : "",
#endif
          } {
    }

    void write(Buffer data, auto cb) {
      auto len = std::make_shared<common::Buffer>(
          scale::encode<uint32_t>(data.size()).value());
      *writing = std::move(data);
      boost::asio::async_write(
          *socket,
          libp2p::asioBuffer(*len),
          [WEAK_SELF, cb, len](boost::system::error_code ec, size_t) mutable {
            WEAK_LOCK(self);
            if (ec) {
              return cb(ec);
            }
            boost::asio::async_write(
                *self->socket,
                libp2p::asioBuffer(*self->writing),
                [weak_self, cb](boost::system::error_code ec, size_t) mutable {
                  WEAK_LOCK(self);
                  if (ec) {
                    return cb(ec);
                  }
                  cb(outcome::success());
                });
          });
    }

    void writeScale(const auto &v, auto cb) {
      write(scale::encode(v).value(), std::move(cb));
    }

    void read(auto cb) {
      auto len = std::make_shared<common::Blob<sizeof(uint32_t)>>();
      boost::asio::async_read(
          *socket,
          libp2p::asioBuffer(*len),
          [WEAK_SELF, cb{std::move(cb)}, len](boost::system::error_code ec,
                                              size_t) mutable {
            WEAK_LOCK(self);
            if (ec) {
              return cb(ec);
            }
            auto len_res = scale::decode<uint32_t>(*len);
            if (len_res.has_error()) {
              return cb(len_res.error());
            }
            self->reading->resize(len_res.value());
            boost::asio::async_read(
                *self->socket,
                libp2p::asioBuffer(*self->reading),
                [cb{std::move(cb)}, reading{self->reading}](
                    boost::system::error_code ec, size_t) mutable {
                  if (ec) {
                    return cb(ec);
                  }
                  cb(std::move(*reading));
                });
          });
    }
  };

  PvfWorkers::PvfWorkers(const application::AppConfiguration &app_config,
                         common::MainThreadPool &main_thread_pool,
                         SecureModeSupport secure_mode_support,
                         std::shared_ptr<libp2p::basic::Scheduler> scheduler)
      : io_context_{main_thread_pool.io_context()},
        main_pool_handler_{main_thread_pool.handlerStarted()},
        scheduler_{std::move(scheduler)},
        exe_{exePath()},
        max_{app_config.pvfMaxWorkers()},
        worker_config_{
            .engine = pvf_runtime_engine(app_config),
            .cache_dir = app_config.runtimeCacheDirPath(),
            .log_params = app_config.log(),
            .force_disable_secure_mode = app_config.disableSecureMode(),
            .secure_mode_support = secure_mode_support,
            .opt_level = app_config.pvfOptimizationLevel()} {
    metrics_registry_->registerGaugeFamily(kMetricQueueSize, "pvf queue size");
    std::unordered_map<PvfExecTimeoutKind, std::string> kind_name{
        {PvfExecTimeoutKind::Approval, "Approval"},
        {PvfExecTimeoutKind::Backing, "Backing"},
    };
    for (auto &[kind, name] : kind_name) {
      metric_queue_size_.emplace(kind,
                                 metrics_registry_->registerGaugeMetric(
                                     kMetricQueueSize, {{"kind", name}}));
    }
  }

  void PvfWorkers::execute(Job &&job) {
    REINVOKE(*main_pool_handler_, execute, std::move(job));
    auto free = findFree(job);
    if (not free.has_value()) {
      if (used_ >= max_) {
        auto &queue = queues_[job.kind];
        queue.emplace_back(std::move(job));
        metric_queue_size_.at(job.kind)->set(queue.size());
        return;
      }
      auto used = std::make_shared<Used>(*this);
      ProcessAndPipes::Config config{};
#if defined(__linux__) && KAGOME_WITH_ASAN
      config.disable_lsan = !worker_config_.force_disable_secure_mode;
#endif
      auto unix_socket_path = filesystem::unique_path(
          std::filesystem::path{worker_config_.cache_dir}
          / "unix_socket.%%%%%%");
      std::error_code ec;
      std::filesystem::remove(unix_socket_path, ec);
      if (ec) {
        job.cb(ec);
        return;
      }
      auto acceptor = std::make_shared<unix::acceptor>(
          *io_context_, unix_socket_path.native());
      auto process = std::make_shared<ProcessAndPipes>(
          *io_context_, exe_, unix_socket_path, config);
      acceptor->async_accept([WEAK_SELF,
                              job{std::move(job)},
                              used,
                              unix_socket_path,
                              acceptor,
                              process{std::move(process)}](
                                 boost::system::error_code ec,
                                 unix::socket &&socket) mutable {
        std::error_code ec2;
        std::filesystem::remove(unix_socket_path, ec2);
        WEAK_LOCK(self);
        if (ec) {
          job.cb(ec);
          return;
        }
        process->socket = std::move(socket);
        process->writeScale(
            self->worker_config_,
            [weak_self, job{std::move(job)}, used{std::move(used)}, process](
                outcome::result<void> r) mutable {
              WEAK_LOCK(self);
              if (not r) {
                job.cb(r.error());
                return;
              }
              self->writeCode(std::move(job),
                              {.process = std::move(process), .code_params{}},
                              std::move(used));
            });
      });
      return;
    }
    runJob(free.value(), std::move(job));
  }

  auto PvfWorkers::findFree(const Job &job) -> std::optional<Free::iterator> {
    auto it = std::ranges::find_if(free_, [&](const Worker &worker) {
      return worker.code_params == job.code_params;
    });
    if (it == free_.end()) {
      it = free_.begin();
    }
    if (it == free_.end()) {
      return std::nullopt;
    }
    return it;
  }

  void PvfWorkers::runJob(Free::iterator free_it, Job &&job) {
    auto worker = *free_it;
    free_.erase(free_it);
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
    if (worker.code_params == job.code_params) {
      call(std::move(job), std::move(worker), std::move(used));
      return;
    }
    worker.code_params = job.code_params;
    const PvfWorkerInput input = job.code_params;

    worker.process->writeScale(
        input,
        [WEAK_SELF, job{std::move(job)}, worker, used{std::move(used)}](
            outcome::result<void> r) mutable {
          WEAK_LOCK(self);
          if (not r) {
            job.cb(r.error());
            return;
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
        [cb]() mutable { cb(std::errc::timed_out); }, job.timeout);
    worker.process->writeScale(PvfWorkerInput{job.args},
                               [cb](outcome::result<void> r) mutable {
                                 if (not r) {
                                   cb(r.error());
                                   return;
                                 }
                               });
    worker.process->read(std::move(cb));
  }

  void PvfWorkers::dequeue() {
    for (auto &kind :
         {PvfExecTimeoutKind::Approval, PvfExecTimeoutKind::Backing}) {
      auto &queue = queues_[kind];
      if (queue.empty()) {
        continue;
      }
      auto free = findFree(queue.front());
      if (not free.has_value()) {
        break;
      }
      auto job = std::move(queue.front());
      queue.pop_front();
      metric_queue_size_.at(kind)->set(queue.size());
      runJob(free.value(), std::move(job));
    }
  }
}  // namespace kagome::parachain
