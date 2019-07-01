/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/protocol/identify/identify.hpp"

#include <string>

#include "libp2p/protocol/identify/pb/identify.pb.h"

namespace {
  const std::string kIdentifyProto = "/ipfs/id/1.0.0";
}

namespace libp2p::protocol {
  using kagome::common::Buffer;

  Identify::Identify(HostSPtr host, kagome::common::Logger log)
      : host_{std::move(host)}, log_{std::move(log)} {
    host_->setProtocolHandler(
        kIdentifyProto, [self{shared_from_this()}](auto &&stream) {
          self->handleIdentifyRequest(std::forward<decltype(stream)>(stream));
        });

    // subscribe on new connections: both income and outcome
  }

  void Identify::sendIdentify(StreamSPtr stream) {
    identify::pb::Identify msg;

    // set the protocols we speak on
    for (const auto &proto : host_->router().getSupportedProtocols()) {
      msg.add_protocols(proto);
    }

    // set an address of the other side, so that it knows, which address we used
    // to connect to it
    if (auto remote_addr = stream->remoteMultiaddr()) {
      msg.set_observedaddr(std::string{remote_addr.value().getStringAddress()});
    }

    // set addresses we are listening on
    for (const auto &addr : host_->getListenAddresses()) {
      msg.add_listenaddrs(std::string{addr.getStringAddress()});
    }

    // set our public key
    // if (auto key = host_->key_repo.getKey(host_->peer_repo.getOurPeerId()) {
    //    msg.set_publickey(key);
    // }

    // set versions of Libp2p and our implementation
    msg.set_protocolversion(std::string{host_->getLibp2pVersion()});
    msg.set_agentversion(std::string{host_->getLibp2pClientVersion()});

    // write the resulting Protobuf message
    auto size = msg.ByteSize();
    auto msg_bytes = std::make_shared<Buffer>(size, 0);
    msg.SerializeToArray(msg_bytes->data(), size);
    stream->write(
        *msg_bytes, size,
        [self{shared_from_this()}, stream = std::move(stream), msg_bytes,
         remote_addr = msg.observedaddr()](auto &&res) mutable {
          self->identifySent(std::forward<decltype(res)>(res), stream,
                             std::move(remote_addr));
        });
  }

  void Identify::identifySent(outcome::result<size_t> written_bytes,
                              const StreamSPtr &stream,
                              std::string &&remote_addr) {
    if (!written_bytes) {
      log_->error("cannot write identify message to stream to peer {}: {}",
                  remote_addr, written_bytes.error().message());
    } else {
      log_->info("successfully written an identify message to peer {}",
                 remote_addr);
    }

    // in any case, close the stream
    stream->close(
        [self{shared_from_this()}, a = std::move(remote_addr)](auto &&res) {
          if (!res) {
            self->log_->error("cannot close the stream to peer {}: {}", a,
                              res.error().message());
          }
        });
  }

  void Identify::receiveIdentify(StreamSPtr stream) {}

  void Identify::identifyReceived(StreamSPtr stream) {}
}  // namespace libp2p::protocol
