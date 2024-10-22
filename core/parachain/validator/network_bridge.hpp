/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <memory>
#include <type_traits>
#include "common/main_thread_pool.hpp"
#include "network/impl/stream_engine.hpp"
#include "network/peer_manager.hpp"
#include "network/protocol_base.hpp"
#include "utils/weak_macro.hpp"

namespace kagome::parachain {

  struct NetworkBridge : std::enable_shared_from_this<NetworkBridge> {
    NetworkBridge(
        common::MainThreadPool &main_thread_pool,
        std::shared_ptr<network::PeerManager> peer_manager,
        std::shared_ptr<application::AppStateManager> app_state_manager)
        : main_pool_handler_(main_thread_pool.handler(*app_state_manager)),
          pm_(std::move(peer_manager)) {}

    template <typename MessageType>
    void send_to_peer(const libp2p::peer::PeerId &peer,
                      const std::shared_ptr<network::ProtocolBase> &protocol,
                      const std::shared_ptr<MessageType> &message) {
      REINVOKE(*main_pool_handler_,
               send_to_peer,
               peer,
               protocol,
               std::move(message));
      std::ignore = tryOpenOutgoingStream(
          peer, protocol, [WEAK_SELF, peer, message, protocol]() {
            WEAK_LOCK(self);
            self->pm_->getStreamEngine()->send(peer, protocol, message);
          });
    }

    template <typename ResponseType, typename Protocol>
    void send_response(std::shared_ptr<network::Stream> stream,
                       std::shared_ptr<Protocol> protocol,
                       std::shared_ptr<ResponseType> response) {
      REINVOKE(*main_pool_handler_,
               send_response,
               std::move(stream),
               std::move(protocol),
               std::move(response));
      protocol->writeResponseAsync(std::move(stream), std::move(*response));
    }

    template <typename MessageType, typename Container>
    requires requires { std::is_same_v<typename Container::value_type, libp2p::peer::PeerId>; }
    void send_to_peers(Container peers,
                       const std::shared_ptr<network::ProtocolBase> &protocol,
                       const std::shared_ptr<MessageType> &message) {
      REINVOKE(*main_pool_handler_,
               send_to_peers,
               std::move(peers),
               protocol,
               message);
      for (const auto &peer : peers) {
        send_to_peer(peer, protocol, message);
      }
    }

   private:
    template <typename F>
    bool tryOpenOutgoingStream(const libp2p::peer::PeerId &peer_id,
                               std::shared_ptr<network::ProtocolBase> protocol,
                               F &&callback) {
      auto stream_engine = pm_->getStreamEngine();
      BOOST_ASSERT(stream_engine);

      if (stream_engine->reserveOutgoing(peer_id, protocol)) {
        protocol->newOutgoingStream(
            peer_id,
            [callback = std::forward<F>(callback),
             protocol,
             peer_id,
             wptr{weak_from_this()}](auto &&stream_result) mutable {
              auto self = wptr.lock();
              if (not self) {
                return;
              }

              auto stream_engine = self->pm_->getStreamEngine();
              stream_engine->dropReserveOutgoing(peer_id, protocol);

              if (!stream_result.has_value()) {
                self->logger->trace("Unable to create stream {} with {}: {}",
                                    protocol->protocolName(),
                                    peer_id,
                                    stream_result.error());
                return;
              }

              auto stream = stream_result.value();
              stream_engine->addOutgoing(std::move(stream_result.value()),
                                         protocol);

              std::forward<F>(callback)();
            });
        return true;
      }
      std::forward<F>(callback)();
      return false;
    }

   private:
    log::Logger logger = log::createLogger("NetworkBridge", "parachain");
    std::shared_ptr<PoolHandler> main_pool_handler_;
    std::shared_ptr<network::PeerManager> pm_;
  };

}  // namespace kagome::parachain
