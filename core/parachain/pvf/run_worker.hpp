/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <chrono>

#include <boost/asio.hpp>
#include <boost/process.hpp>
#include <libp2p/basic/scheduler/asio_scheduler_backend.hpp>
#include <libp2p/basic/scheduler/scheduler_impl.hpp>
#include <libp2p/common/asio_buffer.hpp>

#include "common/buffer.hpp"
#include "outcome/outcome.hpp"

namespace kagome::parachain {

  // run(scale::encode())
  // run(make_shared<Buffer>(scale::encode()))

  template <std::invocable<outcome::result<common::Buffer>> Cb>
  void runWorker(boost::asio::io_context &io_context,
                 std::shared_ptr<libp2p::basic::Scheduler> scheduler,
                 std::chrono::seconds timeout,
                 const std::string &exe,
                 common::Buffer input_,
                 Cb cb_) {
    namespace bp = boost::process;
    using EC = boost::system::error_code;

    struct ProcessAndPipes {
      bp::async_pipe pipe_stdin;
      bp::async_pipe pipe_stdout;
      bp::child process;

      ProcessAndPipes(boost::asio::io_context &io_context,
                      const std::string &exe)
          : pipe_stdin{io_context},
            pipe_stdout{io_context},
            process{
                exe,
                bp::args({"pvf-worker"}),
                bp::std_out > pipe_stdout,
                // bp::std_err > bp::null,
                bp::std_in < pipe_stdin,
            } {}
    };
    auto process = std::make_shared<ProcessAndPipes>(io_context, exe);

    auto cb = [cb_ = std::make_shared<std::optional<Cb>>(std::move(cb_)),
               process](outcome::result<common::Buffer> r) mutable {
      if (*cb_) {
        auto cb = std::move(*cb_);
        cb_->reset();
        process->process.terminate();
        (*cb)(std::move(r));
      }
    };

    io_context.post([scheduler, timeout, cb] {
      scheduler->schedule([cb]() mutable { cb(std::errc::timed_out); },
                          timeout);
    });

    auto input = std::make_shared<common::Buffer>(input_);
    auto input_len = std::make_shared<common::Buffer>(
        scale::encode<uint32_t>(input->size()).value());
    fmt::println(
        stderr, "DEBUG: async_write {}", common::hex_lower(*input_len));
    boost::asio::async_write(
        process->pipe_stdin,
        libp2p::asioBuffer(libp2p::BytesIn{*input_len}),
        [cb, process, input_len, input](EC ec, size_t) mutable {
          if (ec) {
            return cb(ec);
          }
          fmt::println(stderr,
                       "DEBUG: async_write {}",
                       common::hex_lower(libp2p::BytesIn{*input}.first(40)));
          boost::asio::async_write(process->pipe_stdin,
                                   libp2p::asioBuffer(libp2p::BytesIn{*input}),
                                   [cb, process, input](EC ec, size_t) mutable {
                                     if (ec) {
                                       return cb(ec);
                                     }
                                     process->pipe_stdin.close();
                                   });
        });

    auto output_len = std::make_shared<common::Blob<sizeof(uint32_t)>>();
    boost::asio::async_read(
        process->pipe_stdout,
        libp2p::asioBuffer(libp2p::BytesOut{*output_len}),
        [cb, process, output_len](EC ec, size_t) mutable {
          if (ec) {
            return cb(ec);
          }
          auto len_res = scale::decode<uint32_t>(*output_len);
          if (len_res.has_error()) {
            return cb(len_res.error());
          }
          auto output = std::make_shared<common::Buffer>(len_res.value());
          boost::asio::async_read(process->pipe_stdout,
                                  libp2p::asioBuffer(libp2p::BytesOut{*output}),
                                  [cb, process, output](EC ec, size_t) mutable {
                                    if (ec) {
                                      return cb(ec);
                                    }
                                    cb(std::move(*output));
                                  });
        });
  }

}  // namespace kagome::parachain
