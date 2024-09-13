#include <libp2p/protocol/kademlia/config.hpp>

#include "authority_discovery/query/rust_kad.hpp"
#include "log/logger.hpp"
#include "network/common.hpp"
#include "utils/get_exe_path.hpp"
#include "utils/process.hpp"

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
    queue_.emplace_back(scale::encode(config_).value());
  }

  void Kad::lookup(BufferView key, Cb cb) {
    requests_.emplace(key, std::move(cb));
    queue_.emplace_back(scale::encode(Request{key, std::nullopt}).value());
    write();
    if (not reading_) {
      reading_ = true;
      read();
    }
  }

  void Kad::put(BufferView key, BufferView value) {
    queue_.emplace_back(scale::encode(Request{key, value}).value());
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
