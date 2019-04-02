/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/muxer/yamux.hpp"

namespace libp2p::muxer {
  connection::MuxedConnection Yamux::attachAsServer(
      const connection::Connection &conn) {
    return
  }

  connection::MuxedConnection Yamux::attachAsClient(
      const connection::Connection &conn) {}

  std::unique_ptr<stream::Stream> Yamux::newStream() {}

  std::vector<multi::Multiaddress> Yamux::getObservedAdrresses() const {}

  boost::optional<common::PeerInfo> Yamux::getPeerInfo() const {}

  void Yamux::setPeerInfo(const common::PeerInfo &info) {}

  void Yamux::write(const common::NetworkMessage &msg) const {}

  boost::optional<common::NetworkMessage> Yamux::read() const {}

  connection::MuxedConnection Yamux::attach(const connection::Connection &conn,
                                            bool isServer) {}
}  // namespace libp2p::muxer
