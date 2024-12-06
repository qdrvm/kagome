/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <deque>
#include <libp2p/basic/cancel.hpp>
#include <libp2p/connection/stream_and_protocol.hpp>
#include <libp2p/peer/peer_id.hpp>
#include <libp2p/peer/stream_protocols.hpp>
#include <random>
#include <unordered_map>
#include <unordered_set>

#include "common/buffer.hpp"

namespace libp2p {
  struct Host;
}  // namespace libp2p

namespace libp2p::basic {
  class Scheduler;
  class MessageReadWriterUvarint;
}  // namespace libp2p::basic

namespace kagome {
  class PoolHandler;
}  // namespace kagome

namespace kagome::common {
  class MainThreadPool;
}  // namespace kagome::common

namespace kagome::network::notifications {
  using common::MainThreadPool;
  using libp2p::Cancel;
  using libp2p::Host;
  using libp2p::PeerId;
  using libp2p::StreamAndProtocol;
  using libp2p::StreamAndProtocolOrError;
  using libp2p::StreamProtocols;
  using libp2p::basic::MessageReadWriterUvarint;
  using libp2p::basic::Scheduler;
  using libp2p::connection::Stream;
  using ProtocolsGroups = std::vector<StreamProtocols>;

  /**
   * Contains stream, framing, used protocol.
   */
  struct StreamInfo {
    StreamInfo(const ProtocolsGroups &protocols_groups,
               const StreamAndProtocol &info);

    size_t protocol_group;
    std::shared_ptr<Stream> stream;
    std::shared_ptr<MessageReadWriterUvarint> framing;
  };

  /**
   * Closes stream when destroyed.
   */
  struct StreamInfoClose : StreamInfo {
    StreamInfoClose(StreamInfo &&info);
    StreamInfoClose(StreamInfoClose &&) noexcept = default;
    StreamInfoClose &operator=(StreamInfoClose &&) = default;
    StreamInfoClose(const StreamInfoClose &) noexcept = delete;
    StreamInfoClose &operator=(const StreamInfoClose &) = delete;
    ~StreamInfoClose();
  };

  /**
   * State for opening outgoing stream.
   */
  struct PeerOutOpening {};
  /**
   * State for open outgoing stream.
   */
  struct PeerOutOpen {
    PeerOutOpen(StreamInfoClose &&stream);

    StreamInfoClose stream;
    bool writing;
    std::deque<std::shared_ptr<Buffer>> queue;
  };
  /**
   * State for backed off outgoing stream.
   */
  struct PeerOutBackoff {
    Cancel timer;
  };
  /**
   * State for outgoing stream.
   */
  using PeerOut = std::variant<PeerOutOpening, PeerOutOpen, PeerOutBackoff>;

  /**
   * Provides handshake and event callbacks.
   */
  struct Controller {
    virtual ~Controller() = default;
    /**
     * @returns handshake to send
     */
    virtual Buffer handshake() = 0;
    /**
     * Stream with peer was opened/accepted.
     * @returns false to close stream
     */
    virtual bool onHandshake(const PeerId &peer_id,
                             size_t protocol_group,
                             bool out,
                             Buffer &&handshake) = 0;
    /**
     * Message was received from peer.
     * @returns false to close stream
     */
    virtual bool onMessage(const PeerId &peer_id,
                           size_t protocol_group,
                           Buffer &&message) = 0;
    /**
     * Both incoming/outgoing streams with peer were closed.
     */
    virtual void onClose(const PeerId &peer_id) = 0;

    virtual void onClose2(const PeerId &peer_id, bool out) {}
  };

  /**
   * Implements notification protocol behavior.
   */
  class Protocol : public std::enable_shared_from_this<Protocol> {
   public:
    Protocol(MainThreadPool &main_thread_pool,
             std::shared_ptr<Host> host,
             std::shared_ptr<Scheduler> scheduler,
             ProtocolsGroups protocols_groups,
             size_t limit_in,
             size_t limit_out);

    /**
     * Part of constructor:
     * 1. `weak_from_this()` doesn't work in constructor.
     * 2. possible circular reference with controller.
     */
    void start(std::weak_ptr<Controller> controller);
    using PeersOutCb =
        std::function<bool(const PeerId &, size_t protocol_group)>;
    /**
     * Get protocol used by peer.
     */
    std::optional<size_t> peerOut(const PeerId &peer_id);
    /**
     * Visit peers and protocols they use.
     * Used for broadcast.
     */
    void peersOut(const PeersOutCb &cb) const;
    /**
     * Write message with specified protocol to peer.
     * Message is ignored if peer protocol doesn't match.
     */
    void write(const PeerId &peer_id,
               size_t protocol_group,
               std::shared_ptr<Buffer> message);
    /**
     * Write message to peer.
     * Expects single protocol.
     */
    void write(const PeerId &peer_id, std::shared_ptr<Buffer> message);
    /**
     * Add/remove peer to reserved set.
     * Reserved peers are not affected by limits.
     * Streams are automatically opened to reserved peers.
     */
    void reserve(const PeerId &peer_id, bool add);

   private:
    void onError(const PeerId &peer_id, bool out);
    std::chrono::milliseconds backoffTime();
    void backoff(const PeerId &peer_id);
    void onBackoff(const PeerId &peer_id);
    void open(const PeerId &peer_id);
    void onStream(const PeerId &peer_id,
                  const StreamAndProtocol &info,
                  bool out);
    void onHandshake(const PeerId &peer_id,
                     bool out,
                     Buffer &&handshake,
                     StreamInfoClose &&stream);
    void write(const PeerId &peer_id, bool writer);
    void read(const PeerId &peer_id);
    void onMessage(const PeerId &peer_id,
                   size_t protocol_group,
                   Buffer &&message);
    void timer();
    void onTimer();
    size_t peerCount(bool out);
    bool shouldAccept(const PeerId &peer_id);

    std::shared_ptr<PoolHandler> main_pool_handler_;
    std::shared_ptr<Host> host_;
    PeerId own_peer_id_;
    std::shared_ptr<Scheduler> scheduler_;
    ProtocolsGroups protocols_groups_;
    size_t limit_in_;
    size_t limit_out_;
    StreamProtocols protocols_;
    std::weak_ptr<Controller> controller_;
    Cancel timer_;
    std::default_random_engine random_;
    std::unordered_map<PeerId, PeerOut> peers_out_;
    std::unordered_map<PeerId, StreamInfoClose> peers_in_;
    std::unordered_set<PeerId> reserved_;
  };

  /**
   * Contains common constructor parameters for `Protocol` to avoid specifying
   * them explicitly.
   */
  class Factory {
   public:
    Factory(std::shared_ptr<MainThreadPool> main_thread_pool,
            std::shared_ptr<Host> host,
            std::shared_ptr<Scheduler> scheduler);
    /**
     * Make protocol with specified protocols and limits.
     */
    std::shared_ptr<Protocol> make(ProtocolsGroups protocols_groups,
                                   size_t limit_in,
                                   size_t limit_out) const;

   private:
    std::shared_ptr<MainThreadPool> main_thread_pool_;
    std::shared_ptr<Host> host_;
    std::shared_ptr<Scheduler> scheduler_;
  };
}  // namespace kagome::network::notifications
