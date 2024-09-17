/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "parachain/pvf/workers.hpp"

#include <boost/asio/buffered_read_stream.hpp>
#include <boost/asio/buffered_write_stream.hpp>
#include <boost/process.hpp>
#include <libp2p/basic/scheduler.hpp>
#include <libp2p/common/asio_buffer.hpp>
#include <qtils/option_take.hpp>

#include "application/app_configuration.hpp"
#include "common/main_thread_pool.hpp"
#include "parachain/pvf/pvf_worker_types.hpp"
#include "utils/get_exe_path.hpp"
#include "utils/weak_macro.hpp"

namespace kagome::parachain {
  struct AsyncPipe : boost::process::async_pipe {
    using async_pipe::async_pipe;
    using lowest_layer_type = AsyncPipe;
  };

  struct ProcessAndPipes : std::enable_shared_from_this<ProcessAndPipes> {
    AsyncPipe pipe_stdin;
    boost::asio::buffered_write_stream<AsyncPipe &> writer;
    AsyncPipe pipe_stdout;
    boost::asio::buffered_read_stream<AsyncPipe &> reader;
    boost::process::child process;
    std::shared_ptr<Buffer> writing = std::make_shared<Buffer>();
    std::shared_ptr<Buffer> reading = std::make_shared<Buffer>();

    ProcessAndPipes(boost::asio::io_context &io_context, const std::string &exe)
        : pipe_stdin{io_context},
          writer{pipe_stdin},
          pipe_stdout{io_context},
          reader{pipe_stdout},
          process{
              exe,
              boost::process::args({"pvf-worker"}),
              boost::process::std_out > pipe_stdout,
              boost::process::std_in < pipe_stdin,
          } {}

    void write(Buffer data, auto cb) {
      auto len = std::make_shared<common::Buffer>(
          scale::encode<uint32_t>(data.size()).value());
      *writing = std::move(data);
      boost::asio::async_write(
          writer,
          libp2p::asioBuffer(*len),
          [WEAK_SELF, cb, len](boost::system::error_code ec, size_t) mutable {
            WEAK_LOCK(self);
            if (ec) {
              return cb(ec);
            }
            boost::asio::async_write(
                self->writer,
                libp2p::asioBuffer(*self->writing),
                [weak_self, cb](boost::system::error_code ec, size_t) mutable {
                  WEAK_LOCK(self);
                  if (ec) {
                    return cb(ec);
                  }
                  self->writer.async_flush(
                      [cb](boost::system::error_code ec, size_t) mutable {
                        if (ec) {
                          return cb(ec);
                        }
                        cb(outcome::success());
                      });
                });
          });
    }

    void writeScale(const auto &v, auto cb) {
      write(scale::encode(v).value(), std::move(cb));
    }

    void read(auto cb) {
      auto len = std::make_shared<common::Blob<sizeof(uint32_t)>>();
      boost::asio::async_read(
          reader,
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
                self->reader,
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
                         std::shared_ptr<libp2p::basic::Scheduler> scheduler)
      : io_context_{main_thread_pool.io_context()},
        main_pool_handler_{main_thread_pool.handlerStarted()},
        scheduler_{std::move(scheduler)},
        exe_{exePath()},
        max_{app_config.pvfMaxWorkers()},
        timeout_{app_config.pvfSubprocessDeadline()},
        worker_config_{
            .engine = pvf_runtime_engine(app_config),
            .cache_dir = app_config.runtimeCacheDirPath(),
            .log_params = app_config.log(),
            .force_disable_secure_mode = app_config.disableSecureMode(),
        } {}

  void PvfWorkers::execute(Job &&job) {
    REINVOKE(*main_pool_handler_, execute, std::move(job));
    if (free_.empty()) {
      if (used_ >= max_) {
        queue_.emplace(std::move(job));
        return;
      }
      auto used = std::make_shared<Used>(*this);
      auto process = std::make_shared<ProcessAndPipes>(*io_context_, exe_);
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
    auto it = std::ranges::find_if(free_, [&](const Worker &worker) {
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
