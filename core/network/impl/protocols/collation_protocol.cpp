/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "network/impl/protocols/collation_protocol.hpp"

#include "network/common.hpp"
#include "network/helpers/peer_id_formatter.hpp"
#include "network/helpers/scale_message_read_writer.hpp"
#include "network/impl/protocols/protocol_error.hpp"

namespace kagome::network {

  CollationProtocol::CollationProtocol(
      libp2p::Host &host,
      application::ChainSpec const &chain_spec,
      std::shared_ptr<CollationObserver> observer)
      : host_(host),
        observer_(std::move(observer)),
        protocol_(
            fmt::format(kCollationProtocol.data(), chain_spec.protocolId())),
        log_(log::createLogger("CollationProtocol", "kagome_protocols")) {}

  bool CollationProtocol::start() {
    host_.setProtocolHandler(protocol_, [wp(weak_from_this())](auto &&stream) {
      if (auto self = wp.lock()) {
        if (auto peer_id = stream->remotePeerId()) {
          SL_TRACE(self->log_,
                   "Handled {} protocol stream from: {}",
                   self->protocol_,
                   peer_id.value().toBase58());
          self->onIncomingStream(std::forward<decltype(stream)>(stream));
          return;
        }
        self->log_->warn("Handled {} protocol stream from unknown peer",
                         self->protocol_);
        stream->reset();
      }
    });
    return true;
  }

  bool CollationProtocol::stop() {
    return true;
  }

  void CollationProtocol::newOutgoingStream(
      const PeerInfo &peer_info,
      std::function<void(outcome::result<std::shared_ptr<Stream>>)> &&cb) {
    /// This protocol is used for INCOMING streams only from the collators.
    /// Collators notify validators when they ready for collating and have data
    /// for a new block.
    assert(!"For collation protocol should not be used.");
  }

}  // namespace kagome::network