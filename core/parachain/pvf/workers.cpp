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
#include <libp2p/common/final_action.hpp>
#include <qtils/option_take.hpp>

#include "application/app_configuration.hpp"
#include "common/main_thread_pool.hpp"
#include "coro/asio.hpp"
#include "coro/spawn.hpp"
#include "filesystem/common.hpp"
#include "parachain/pvf/pvf_worker_types.hpp"
#include "utils/get_exe_path.hpp"
#include "utils/weak_macro.hpp"

namespace kagome::parachain {
  using unix = boost::asio::local::stream_protocol;

  constexpr auto kMetricQueueSize = "kagome_pvf_queue_size";

  inline outcome::result<void> tryRemove(const std::filesystem::path &path) {
    std::error_code ec;
    std::filesystem::remove(path, ec);
    if (ec) {
      return ec;
    }
    return outcome::success();
  }

  struct ProcessAndPipes {
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
#ifdef KAGOME_WITH_ASAN
              boost::process::env["ASAN_OPTIONS"] =
                  config.disable_lsan ? "detect_leaks=0" : "",
#endif
          } {
    }

    void close() {
      boost::system::error_code ec;
      process.terminate(ec);
      socket.reset();
    }

    CoroOutcome<void> write(Buffer data) {
      auto len = scale::encode<uint32_t>(data.size()).value();
      CO_TRY(co_await coroWrite(*socket, len));
      CO_TRY(co_await coroWrite(*socket, data));
      co_return outcome::success();
    }

    CoroOutcome<void> writeScale(const auto &v) {
      CO_TRY(co_await write(scale::encode(v).value()));
      co_return outcome::success();
    }

    CoroOutcome<Buffer> read() {
      common::Blob<sizeof(uint32_t)> len_buf;
      CO_TRY(co_await coroRead(*socket, len_buf));
      auto len = CO_TRY(scale::decode<uint32_t>(len_buf));
      Buffer buf;
      buf.resize(len);
      CO_TRY(co_await coroRead(*socket, buf));
      co_return buf;
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
        } {
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
    coroSpawn(io_context_->get_executor(),
              [self{shared_from_this()}, job{std::move(job)}]() -> Coro<void> {
                co_await self->tryExecute(job);
              });
  }

  Coro<void> PvfWorkers::tryExecute(const Job &job) {
    auto worker = findFree(job);
    if (not worker and used_ >= max_) {
      auto &queue = queues_[job.kind];
      queue.emplace_back(std::move(job));
      metric_queue_size_.at(job.kind)->set(queue.size());
      co_return;
    }
    auto r = co_await execute(worker, job);
    if (not r and worker) {
      worker->process->close();
    }
    job.cb(r);
  }

  CoroOutcome<Buffer> PvfWorkers::execute(std::optional<Worker> &worker,
                                          const Job &job) {
    Used used{*this};
    if (not worker) {
      worker = CO_TRY(co_await newWorker());
    }
    if (worker->code_params != job.code_params) {
      worker->code_params = job.code_params;
      CO_TRY(co_await worker->process->writeScale(
          PvfWorkerInput{job.code_params}));
    }
    auto timeout = scheduler_->scheduleWithHandle(
        [process{worker->process}]() mutable { process->close(); },
        job.timeout);
    CO_TRY(co_await worker->process->writeScale(PvfWorkerInput{job.args}));
    auto output = CO_TRY(co_await worker->process->read());
    free_.emplace_back(std::move(*worker));
    dequeue();
    co_return output;
  }

  std::optional<PvfWorkers::Worker> PvfWorkers::findFree(const Job &job) {
    auto it = std::ranges::find_if(free_, [&](const Worker &worker) {
      return worker.code_params == job.code_params;
    });
    if (it == free_.end()) {
      it = free_.begin();
    }
    if (it == free_.end()) {
      return std::nullopt;
    }
    auto worker = std::move(*it);
    free_.erase(it);
    return std::move(worker);
  }

  CoroOutcome<PvfWorkers::Worker> PvfWorkers::newWorker() {
    ProcessAndPipes::Config config{};
#if defined(__linux__) && defined(KAGOME_WITH_ASAN)
    config.disable_lsan = !worker_config_.force_disable_secure_mode;
#endif
    auto unix_socket_path = filesystem::unique_path(
        std::filesystem::path{worker_config_.cache_dir} / "unix_socket.%%%%%%");
    CO_TRY(tryRemove(unix_socket_path));
    auto executor = co_await boost::asio::this_coro::executor;
    unix::acceptor acceptor{executor, unix_socket_path.native()};
    auto cleanup = std::make_optional(libp2p::common::MovableFinalAction{[&] {
      acceptor.close();
      std::ignore = tryRemove(unix_socket_path);
    }});
    auto process = std::make_shared<ProcessAndPipes>(
        *io_context_, exe_, unix_socket_path, config);
    process->socket =
        CO_TRY(coroOutcome(co_await acceptor.async_accept(useCoroOutcome)));
    cleanup.reset();
    CO_TRY(co_await process->writeScale(worker_config_));
    co_return Worker{.process = std::move(process)};
  }

  PvfWorkers::Used::Used(PvfWorkers &self) : weak_self{self.weak_from_this()} {
    ++self.used_;
  }

  PvfWorkers::Used::~Used() {
    IF_WEAK_LOCK(self) {
      --self->used_;
    }
  }

  void PvfWorkers::dequeue() {
    for (auto &kind :
         {PvfExecTimeoutKind::Approval, PvfExecTimeoutKind::Backing}) {
      auto &queue = queues_[kind];
      if (queue.empty()) {
        continue;
      }
      auto job = std::move(queue.front());
      queue.pop_front();
      metric_queue_size_.at(kind)->set(queue.size());
      execute(std::move(job));
    }
  }
}  // namespace kagome::parachain
