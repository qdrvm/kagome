#include <libp2p/protocol/kademlia/config.hpp>

#include <boost/process.hpp>
#include <libp2p/common/asio_buffer.hpp>

#include "authority_discovery/query/rust_kad.hpp"
#include "log/logger.hpp"
#include "network/common.hpp"
#include "utils/get_exe_path.hpp"
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
          ::scale::encode<uint32_t>(data.size()).value());
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
      write(::scale::encode(v).value(), std::move(cb));
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

namespace kagome::rust_kad {
  Kad::Kad(std::shared_ptr<boost::asio::io_context> io_context)
      : log_{log::createLogger("RustKad")},
        io_context_{std::move(io_context)},
        process_{ProcessAndPipes::make(
            *io_context_, exePath().string() + "-rust-kad", {})} {}
  Kad::Kad(std::shared_ptr<boost::asio::io_context> io_context,
           const libp2p::protocol::kademlia::Config &kad_config,
           const application::ChainSpec &chain_spec)
      : Kad{std::move(io_context)} {
    config_.kad_protocols = kad_config.protocols;
    for (auto &addr : chain_spec.bootNodes()) {
      config_.bootstrap.emplace_back(addr.getStringAddress());
    }
    queue_.emplace_back(::scale::encode(config_).value());
  }

  bool Kad::use() const {
    static auto use = [] {
      auto s = getenv("NO_RUST_KAD");
      return not s or std::string_view{s} == "0";
    }();
    return use;
  }

  void Kad::lookup(BufferView key, Cb cb) {
    requests_.emplace(key, std::move(cb));
    queue_.emplace_back(::scale::encode(Request{key, std::nullopt}).value());
    write();
    if (not reading_) {
      reading_ = true;
      read();
    }
  }

  void Kad::put(BufferView key, BufferView value) {
    queue_.emplace_back(::scale::encode(Request{key, value}).value());
    write();
  }

  void Kad::write() {
    if (writing_) {
      return;
    }
    if (queue_.empty()) {
      return;
    }
    writing_ = true;
    auto buf = std::move(queue_.front());
    queue_.pop_front();
    process_->write(buf, [WEAK_SELF](outcome::result<void> r) {
      WEAK_LOCK(self);
      if (not r) {
        SL_ERROR(self->log_, "write(): {}", r.error());
        return self->error();
      }
      self->writing_ = false;
      self->write();
    });
  }

  void Kad::read() {
    process_->readScale<Response>([WEAK_SELF](outcome::result<Response> r) {
      WEAK_LOCK(self);
      if (not r) {
        SL_ERROR(self->log_, "read(): {}", r.error());
        return self->error();
      }
      self->read();
      auto &response = r.value();
      auto it = self->requests_.find(response.key);
      if (it != self->requests_.end()) {
        auto cb = std::move(it->second);
        self->requests_.erase(it);
        cb(std::move(response.values));
      }
    });
  }

  void Kad::error() {
    queue_.clear();
    auto requests = std::exchange(requests_, {});
    for (auto &p : requests) {
      p.second({});
    }
  }
}  // namespace kagome::rust_kad
