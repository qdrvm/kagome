/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/basic/message_read_writer_uvarint.hpp>
#include <libp2p/basic/scheduler.hpp>
#include <libp2p/host/host.hpp>

#include "common/main_thread_pool.hpp"
#include "network/helpers/new_stream.hpp"
#include "network/notifications/handshake.hpp"
#include "network/notifications/protocol.hpp"
#include "utils/map_entry.hpp"
#include "utils/weak_macro.hpp"

namespace kagome::network::notifications {
  constexpr auto kTimer = std::chrono::seconds{1};
  constexpr auto kBackoffMin = std::chrono::seconds{5};
  constexpr auto kBackoffMax = std::chrono::seconds{10};

  // TODO(turuslan): #2359, remove when `YamuxStream::readSome` returns error
  inline bool isClosed(const StreamInfoClose &stream) {
    return stream.stream->isClosed();
  }

  StreamInfo::StreamInfo(const ProtocolsGroups &protocols_groups,
                         const StreamAndProtocol &info)
      : protocol_group{},
        stream{info.stream},
        framing{std::make_shared<MessageReadWriterUvarint>(info.stream)} {
    auto it = std::ranges::find_if(
        protocols_groups, [&](const StreamProtocols &protocols) {
          return std::ranges::find(protocols, info.protocol) != protocols.end();
        });
    protocol_group =
        it == protocols_groups.end() ? 0 : it - protocols_groups.begin();
  }

  StreamInfoClose::StreamInfoClose(StreamInfo &&info)
      : StreamInfo{std::move(info)} {}

  StreamInfoClose::~StreamInfoClose() {
    if (stream) {
      stream->reset();
    }
  }

  PeerOutOpen::PeerOutOpen(StreamInfoClose &&stream)
      : stream{std::move(stream)}, writing{false} {}

  Protocol::Protocol(MainThreadPool &main_thread_pool,
                     std::shared_ptr<Host> host,
                     std::shared_ptr<Scheduler> scheduler,
                     ProtocolsGroups protocols_groups,
                     size_t limit_in,
                     size_t limit_out)
      : main_pool_handler_{main_thread_pool.handlerStarted()},
        host_{std::move(host)},
        own_peer_id_{host_->getId()},
        scheduler_{std::move(scheduler)},
        protocols_groups_{std::move(protocols_groups)},
        limit_in_{limit_in},
        limit_out_{limit_out} {
    for (auto &protocols : protocols_groups_) {
      protocols_.insert(protocols_.end(), protocols.begin(), protocols.end());
    }
  }

  void Protocol::start(std::weak_ptr<Controller> controller) {
    REINVOKE(*main_pool_handler_, start, std::move(controller));
    controller_ = std::move(controller);
    host_->setProtocolHandler(
        protocols_, [WEAK_SELF](const StreamAndProtocol &info) {
          WEAK_LOCK(self);
          auto peer_id = info.stream->remotePeerId().value();
          self->onStream(peer_id, info, false);
        });
    timer();
  }

  std::optional<size_t> Protocol::peerOut(const PeerId &peer_id) {
    EXPECT_THREAD(*main_pool_handler_);
    if (auto peer = entry(peers_out_, peer_id)) {
      if (auto *open = std::get_if<PeerOutOpen>(&*peer)) {
        return open->stream.protocol_group;
      }
    }
    return std::nullopt;
  }

  void Protocol::peersOut(const PeersOutCb &cb) const {
    EXPECT_THREAD(*main_pool_handler_);
    for (auto &[peer_id, peer] : peers_out_) {
      auto *open = std::get_if<PeerOutOpen>(&peer);
      if (not open) {
        continue;
      }
      if (not cb(peer_id, open->stream.protocol_group)) {
        break;
      }
    }
  }

  void Protocol::write(const PeerId &peer_id,
                       size_t protocol_group,
                       std::shared_ptr<Buffer> message) {
    REINVOKE(*main_pool_handler_,
             write,
             peer_id,
             protocol_group,
             std::move(message));
    auto peer = entry(peers_out_, peer_id);
    if (not peer) {
      return;
    }
    auto *open = std::get_if<PeerOutOpen>(&*peer);
    if (not open) {
      return;
    }
    if (open->stream.protocol_group != protocol_group) {
      return;
    }
    open->queue.emplace_back(std::move(message));
    write(peer_id, false);
  }

  void Protocol::write(const PeerId &peer_id, std::shared_ptr<Buffer> message) {
    if (protocols_groups_.size() != 1) {
      throw std::logic_error{"write on ambigous protocol"};
    }
    write(peer_id, 0, std::move(message));
  }

  void Protocol::reserve(const PeerId &peer_id, bool add) {
    REINVOKE(*main_pool_handler_, reserve, peer_id, add);
    if (add) {
      if (not reserved_.emplace(peer_id).second) {
        return;
      }
    } else {
      if (reserved_.erase(peer_id) == 0) {
        return;
      }
    }
  }

  void Protocol::onError(const PeerId &peer_id, bool out) {
    auto closed = false;
    auto peer_out = entry(peers_out_, peer_id);
    auto peer_out_open =
        peer_out and std::holds_alternative<PeerOutOpen>(*peer_out);
    auto peer_in = entry(peers_in_, peer_id);
    if (out) {
      backoff(peer_id);
      if (not peer_out_open) {
        return;
      }
      closed = not peer_in;
    } else {
      if (not peer_in) {
        return;
      }
      peer_in.remove();
      closed = not peer_out_open;
    }
    if (closed) {
      if (auto controller = controller_.lock()) {
        controller->onClose(peer_id);
      }
    }
  }

  std::chrono::milliseconds Protocol::backoffTime() {
    return std::chrono::milliseconds{std::uniform_int_distribution<>{
        std::chrono::milliseconds{kBackoffMin}.count(),
        std::chrono::milliseconds{kBackoffMax}.count(),
    }(random_)};
  }

  void Protocol::backoff(const PeerId &peer_id) {
    auto peer = entry(peers_out_, peer_id);
    if (not peer) {
      return;
    }
    peer.insert_or_assign(PeerOutBackoff{
        scheduler_->scheduleWithHandle(
            [WEAK_SELF, peer_id] {
              WEAK_LOCK(self);
              self->onBackoff(peer_id);
            },
            backoffTime()),
    });
  }

  void Protocol::onBackoff(const PeerId &peer_id) {
    auto peer = entry(peers_out_, peer_id);
    if (not peer) {
      return;
    }
    if (not std::holds_alternative<PeerOutBackoff>(*peer)) {
      return;
    }
    peer.remove();
  }

  void Protocol::open(const PeerId &peer_id) {
    if (peer_id == own_peer_id_) {
      return;
    }
    if (not peers_out_.emplace(peer_id, PeerOutOpening{}).second) {
      return;
    }
    auto cb = [WEAK_SELF, peer_id](StreamAndProtocolOrError r) {
      WEAK_LOCK(self);
      if (not r) {
        self->onError(peer_id, true);
        return;
      }
      self->onStream(peer_id, r.value(), true);
    };
    newStream(*host_, peer_id, protocols_, std::move(cb));
  }

  void Protocol::onStream(const PeerId &peer_id,
                          const StreamAndProtocol &info,
                          bool out) {
    auto reject = [&] { info.stream->reset(); };
    auto controller = controller_.lock();
    if (not controller) {
      reject();
      return;
    }
    if (not out) {
      if (not shouldAccept(peer_id)) {
        reject();
        return;
      }
    }
    StreamInfo stream{protocols_groups_, info};
    auto cb = [WEAK_SELF, peer_id, out, stream](
                  MessageReadWriterUvarint::ReadCallback r) mutable {
      WEAK_LOCK(self);
      if (not r) {
        self->onError(peer_id, out);
        return;
      }
      self->onHandshake(peer_id,
                        out,
                        std::move(*r.value()),
                        StreamInfoClose{std::move(stream)});
    };
    handshakeRaw(
        stream.stream, stream.framing, controller->handshake(), std::move(cb));
  }

  void Protocol::onHandshake(const PeerId &peer_id,
                             bool out,
                             Buffer &&handshake,
                             StreamInfoClose &&stream) {
    auto protocol_group = stream.protocol_group;
    auto controller = controller_.lock();
    if (not controller) {
      return;
    }
    if (out) {
      auto peer = entry(peers_out_, peer_id);
      if (not peer) {
        return;
      }
      if (not std::holds_alternative<PeerOutOpening>(*peer)) {
        return;
      }
      auto buffer = std::make_shared<Buffer>(1);
      stream.stream->read(
          *buffer,
          buffer->size(),
          [WEAK_SELF, peer_id, buffer](outcome::result<size_t>) {
            WEAK_LOCK(self);
            self->onError(peer_id, true);
          });
      *peer = PeerOutOpen{std::move(stream)};
    } else {
      if (not shouldAccept(peer_id)) {
        return;
      }
    }
    if (not controller->onHandshake(
            peer_id, protocol_group, out, std::move(handshake))) {
      onError(peer_id, out);
      return;
    }
    if (not out) {
      peers_in_.emplace(peer_id, std::move(stream));
      open(peer_id);
      read(peer_id);
    }
  }

  void Protocol::write(const PeerId &peer_id, bool writer) {
    auto peer = entry(peers_out_, peer_id);
    if (not peer) {
      return;
    }
    auto *open = std::get_if<PeerOutOpen>(&*peer);
    if (not open) {
      return;
    }
    if (not writer and open->writing) {
      return;
    }
    if (open->queue.empty()) {
      if (writer) {
        open->writing = false;
      }
      return;
    }
    open->writing = true;
    auto message = std::move(open->queue.front());
    open->queue.pop_front();
    auto cb = [WEAK_SELF, peer_id](outcome::result<size_t> r) {
      WEAK_LOCK(self);
      if (not r) {
        self->onError(peer_id, true);
        return;
      }
      self->write(peer_id, true);
    };
    // MessageReadWriterUvarint copies message
    open->stream.framing->write(*message, std::move(cb));
  }

  void Protocol::read(const PeerId &peer_id) {
    auto stream = entry(peers_in_, peer_id);
    if (not stream) {
      return;
    }
    if (isClosed(*stream)) {
      onError(peer_id, false);
      return;
    }
    auto cb = [WEAK_SELF, peer_id, protocol_group{stream->protocol_group}](
                  libp2p::basic::MessageReadWriter::ReadCallback r) mutable {
      WEAK_LOCK(self);
      if (not r) {
        self->onError(peer_id, false);
        return;
      }
      auto &message = r.value();
      self->onMessage(peer_id,
                      protocol_group,
                      // TODO(turuslan): `MessageReadWriterUvarint` reuse buffer
                      message ? Buffer{std::move(*message)} : Buffer{});
    };
    stream->framing->read(std::move(cb));
  }

  void Protocol::onMessage(const PeerId &peer_id,
                           size_t protocol,
                           Buffer &&message) {
    auto controller = controller_.lock();
    if (not controller) {
      onError(peer_id, false);
      return;
    }
    if (not controller->onMessage(peer_id, protocol, std::move(message))) {
      onError(peer_id, false);
      return;
    }
    read(peer_id);
  }

  void Protocol::timer() {
    timer_ = scheduler_->scheduleWithHandle(
        [WEAK_SELF] {
          WEAK_LOCK(self);
          self->onTimer();
          self->timer();
        },
        kTimer);
  }

  void Protocol::onTimer() {
    if (controller_.expired()) {
      return;
    }
    for (auto it = peers_in_.begin(); it != peers_in_.end();) {
      auto &[peer_id, stream] = *it;
      ++it;
      if (isClosed(stream)) {
        // copy `it->first` before `erase(it)`
        onError(PeerId{peer_id}, false);
      }
    }
    for (auto &peer_id : reserved_) {
      open(peer_id);
    }
    auto count = peerCount(true);
    if (count >= limit_out_) {
      return;
    }
    for (auto &conn :
         host_->getNetwork().getConnectionManager().getConnections()) {
      if (conn->isClosed()) {
        continue;
      }
      auto peer_id = conn->remotePeer().value();
      if (reserved_.contains(peer_id)) {
        continue;
      }
      if (peers_out_.contains(peer_id)) {
        continue;
      }
      open(conn->remotePeer().value());
      ++count;
      if (count >= limit_out_) {
        break;
      }
    }
  }

  size_t Protocol::peerCount(bool out) {
    size_t count = 0;
    if (not out) {
      for (auto &p : peers_in_) {
        if (isClosed(p.second)) {
          continue;
        }
        if (reserved_.contains(p.first)) {
          continue;
        }
        ++count;
      }
    } else {
      for (auto &[peer_id, peer] : peers_out_) {
        if (reserved_.contains(peer_id)) {
          continue;
        }
        if (std::holds_alternative<PeerOutBackoff>(peer)) {
          continue;
        }
        ++count;
      }
    }
    return count;
  }

  bool Protocol::shouldAccept(const PeerId &peer_id) {
    auto peer_in = entry(peers_in_, peer_id);
    if (peer_in) {
      if (isClosed(*peer_in)) {
        onError(peer_id, false);
      } else {
        return false;
      }
    }
    if (reserved_.contains(peer_id)) {
      return true;
    }
    auto peer_out = entry(peers_out_, peer_id);
    if (peer_out and not std::holds_alternative<PeerOutBackoff>(*peer_out)) {
      return true;
    }
    return peerCount(false) < limit_in_;
  }

  Factory::Factory(std::shared_ptr<MainThreadPool> main_thread_pool,
                   std::shared_ptr<Host> host,
                   std::shared_ptr<Scheduler> scheduler)
      : main_thread_pool_{std::move(main_thread_pool)},
        host_{std::move(host)},
        scheduler_{std::move(scheduler)} {}

  std::shared_ptr<Protocol> Factory::make(ProtocolsGroups protocols_groups,
                                          size_t limit_in,
                                          size_t limit_out) const {
    return std::make_shared<Protocol>(*main_thread_pool_,
                                      host_,
                                      scheduler_,
                                      std::move(protocols_groups),
                                      limit_in,
                                      limit_out);
  }
}  // namespace kagome::network::notifications
