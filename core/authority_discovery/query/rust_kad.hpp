#pragma once

#include <deque>
#include <unordered_map>

#include "common/buffer.hpp"
#include "scale/tie.hpp"

namespace boost::asio {
  class io_context;
}  // namespace boost::asio

namespace libp2p::protocol::kademlia {
  class Config;
}  // namespace libp2p::protocol::kademlia

namespace soralog {
  class Logger;
}  // namespace soralog

namespace kagome {
  struct ProcessAndPipes;
}  // namespace kagome

namespace kagome::application {
  class ChainSpec;
}  // namespace kagome::application

namespace kagome::blockchain {
  class GenesisBlockHash;
}  // namespace kagome::blockchain

namespace kagome::log {
  using Logger = std::shared_ptr<soralog::Logger>;
}  // namespace kagome::log

namespace kagome::rust_kad {
  struct Config {
    SCALE_TIE(4);
    std::vector<std::string> kad_protocols;
    std::vector<std::string> bootstrap;
    uint32_t quorum = 4;
    uint32_t connection_idle = 60;
  };
  struct Request {
    SCALE_TIE(2);
    Buffer key;
    std::optional<Buffer> put_value;
  };
  struct Response {
    SCALE_TIE(2);
    Buffer key;
    std::vector<Buffer> values;
  };

  struct Kad : /* BUG */ public std::enable_shared_from_this<Kad> {
    using Cb = std::function<void(std::vector<Buffer>)>;

    Kad(std::shared_ptr<boost::asio::io_context> io_context);
    Kad(std::shared_ptr<boost::asio::io_context> io_context,
        const libp2p::protocol::kademlia::Config &kad_config,
        const application::ChainSpec &chain_spec);

    bool use() const;

    void lookup(BufferView key, Cb cb);
    void put(BufferView key, BufferView value);

    void write();
    void read();
    void error();

    log::Logger log_;
    std::shared_ptr<boost::asio::io_context> io_context_;
    std::shared_ptr<ProcessAndPipes> process_;
    Config config_;
    std::unordered_map<Buffer, Cb> requests_;
    std::deque<Buffer> queue_;
    bool writing_ = false;
    bool reading_ = false;
  };
}  // namespace kagome::rust_kad
