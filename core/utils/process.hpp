/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <boost/asio/buffered_read_stream.hpp>
#include <boost/asio/buffered_write_stream.hpp>
#include <boost/process.hpp>
#include <libp2p/common/asio_buffer.hpp>

#include "common/buffer.hpp"
#include "utils/weak_macro.hpp"

namespace kagome {
  struct AsyncPipe : boost::process::async_pipe {
    using async_pipe::async_pipe;
    using lowest_layer_type = AsyncPipe;
  };

  struct ProcessAndPipes : std::enable_shared_from_this<ProcessAndPipes> {
    AsyncPipe pipe_stdin;
    AsyncPipe &writer;
    AsyncPipe pipe_stdout;
    AsyncPipe &reader;
    boost::process::child process;
    std::shared_ptr<Buffer> writing = std::make_shared<Buffer>();
    std::shared_ptr<Buffer> reading = std::make_shared<Buffer>();

    ProcessAndPipes(boost::asio::io_context &io_context,
                    const std::string &exe,
                    const std::vector<std::string> &args)
        : pipe_stdin{io_context},
          writer{pipe_stdin},
          pipe_stdout{io_context},
          reader{pipe_stdout},
          process{
              exe,
              boost::process::args(args),
              boost::process::std_out > pipe_stdout,
              boost::process::std_in < pipe_stdin,
          } {}

    static auto make(boost::asio::io_context &io_context,
                     const std::string &exe,
                     const std::vector<std::string> &args) {
      return std::make_shared<ProcessAndPipes>(io_context, exe, args);
    }

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

    template <typename T>
    void readScale(auto cb) {
      read([cb{std::move(cb)}](outcome::result<Buffer> r) mutable {
        if (not r) {
          return cb(r.error());
        }
        cb(scale::decode<T>(r.value()));
      });
    }
  };
}  // namespace kagome
