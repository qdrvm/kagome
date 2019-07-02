/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/protocol/identify/identify.hpp"

#include <string>
#include <tuple>

#include "common/hexutil.hpp"
#include "libp2p/network/connection_manager.hpp"
#include "libp2p/network/network.hpp"
#include "libp2p/peer/address_repository.hpp"
#include "libp2p/protocol/identify/pb/identify.pb.h"

namespace {
  const std::string kIdentifyProto = "/ipfs/id/1.0.0";

  /**
   * Get a tuple of stringified <PeerId, Multiaddress> of the peer the (\param
   * stream) is connected to
   */
  std::tuple<std::string, std::string> getPeerIdentity(
      const std::shared_ptr<libp2p::connection::Stream> &stream) {
    std::string id = "unknown", addr = "unknown";
    if (auto id_res = stream->remotePeerId()) {
      id = id_res.value().toBase58();
    }
    if (auto addr_res = stream->remoteMultiaddr()) {
      addr = addr_res.value().getStringAddress();
    }
    return {std::move(id), std::move(addr)};
  }
}  // namespace

namespace libp2p::protocol {
  using kagome::common::Buffer;

  Identify::Identify(
      HostSPtr host, event::Bus &event_bus,
      peer::IdentityManager &identity_manager,
      std::shared_ptr<crypto::marshaller::KeyMarshaller> key_marshaller,
      kagome::common::Logger log)
      : host_{std::move(host)},
        bus_{event_bus},
        identity_manager_{identity_manager},
        key_marshaller_{std::move(key_marshaller)},
        log_{std::move(log)} {
    // firstly, we want to request identify for any connection we or the other
    // side established
    bus_.getChannel<network::event::OnNewConnectionChannel>().subscribe(
        [self{shared_from_this()}](auto &&conn) {
          self->onNewConnection(conn);
        });

    // secondly, we need to handle cases, when a stream with Identify protocol
    // is created from the other side - that side wants to know our info
    host_->setProtocolHandler(
        kIdentifyProto, [self{weak_from_this()}](auto &&stream) {
          if (self.expired()) {
            return;
          }
          self.lock()->sendIdentify(std::forward<decltype(stream)>(stream));
        });
  }

  void Identify::onNewConnection(
      const std::weak_ptr<connection::CapableConnection> &conn) {
    if (conn.expired()) {
      log_->error("received a dead weak ptr in NewConnectionEvent; strange");
      return;
    }

    auto remote_peer_res = conn.lock()->remotePeer();
    if (!remote_peer_res) {
      log_->error(
          "cannot get a remote peer id from the received connection: {}",
          remote_peer_res.error().message());
      return;
    }

    auto remote_peer_addr_res = conn.lock()->remoteMultiaddr();
    if (!remote_peer_addr_res) {
      log_->error(
          "cannot get a remote peer address from the received connection: {}",
          remote_peer_addr_res.error().message());
      return;
    }

    peer::PeerInfo peer_info{std::move(remote_peer_res.value()),
                             std::vector<multi::Multiaddress>{
                                 std::move(remote_peer_addr_res.value())}};

    auto res = host_->newStream(
        std::move(peer_info), kIdentifyProto,
        [self{shared_from_this()}](auto &&stream_res) {
          if (!stream_res) {
            self->log_->error(
                "cannot create a stream over a received connection: {}",
                stream_res.error().message());
            return;
          }
          self->receiveIdentify(std::move(stream_res.value()));
        });
    if (!res) {
      log_->error("cannot create a stream over a received connection: {}",
                  res.error().message());
    }
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
    auto marshalled_pubkey_res =
        key_marshaller_->marshal(identity_manager_.getKeyPair().publicKey);
    if (!marshalled_pubkey_res) {
      log_->critical(
          "cannot marshal public key, which was provided to us by the identity "
          "manager: {}",
          marshalled_pubkey_res.error().message());
    } else {
      auto &&marshalled_pubkey = marshalled_pubkey_res.value();
      msg.set_publickey(marshalled_pubkey.data(), marshalled_pubkey.size());
    }

    // set versions of Libp2p and our implementation
    msg.set_protocolversion(std::string{host_->getLibp2pVersion()});
    msg.set_agentversion(std::string{host_->getLibp2pClientVersion()});

    // write the resulting Protobuf message
    auto size = msg.ByteSize();
    auto msg_bytes = std::make_shared<Buffer>(size, 0);
    msg.SerializeToArray(msg_bytes->data(), size);
    stream->write(*msg_bytes, size,
                  [self{shared_from_this()}, stream = std::move(stream),
                   msg_bytes](auto &&res) mutable {
                    self->identifySent(std::forward<decltype(res)>(res),
                                       stream);
                  });
  }

  void Identify::identifySent(outcome::result<size_t> written_bytes,
                              const StreamSPtr &stream) {
    auto [peer_id, peer_addr] = getPeerIdentity(stream);
    if (!written_bytes) {
      log_->error("cannot write identify message to stream to peer {}, {}: {}",
                  peer_id, peer_addr, written_bytes.error().message());
      return stream->reset([](auto &&) {});
    }

    log_->info("successfully written an identify message to peer {}, {}",
               peer_id, peer_addr);

    stream->close([self{shared_from_this()}, p = std::move(peer_id),
                   a = std::move(peer_addr)](auto &&res) {
      if (!res) {
        self->log_->error("cannot close the stream to peer {}, {}: {}", p, a,
                          res.error().message());
      }
    });
  }

  void Identify::receiveIdentify(StreamSPtr stream) {
    static constexpr size_t kMaxMessageSize = 2048;  // from Go implementation

    auto msg_buf = std::make_shared<Buffer>(kMaxMessageSize, 0);
    return stream->readSome(
        *msg_buf, kMaxMessageSize,
        [self{shared_from_this()}, s = std::move(stream), msg_buf](auto &&res) {
          self->identifyReceived(std::forward<decltype(res)>(res), s, msg_buf);
        });
  }

  void Identify::identifyReceived(outcome::result<size_t> read_bytes,
                                  const StreamSPtr &stream,
                                  const BufSPtr &buffer) {
    auto [peer_id_str, peer_addr_str] = getPeerIdentity(stream);
    if (!read_bytes) {
      log_->error("cannot read an identify message from peer {}, {}: {}",
                  peer_id_str, peer_addr_str, read_bytes.error());
      return stream->reset([](auto &&) {});
    }

    log_->info("received an identify message from peer {}, {}", peer_id_str,
               peer_addr_str);
    stream->close([self{shared_from_this()}, p = std::move(peer_id_str),
                   a = std::move(peer_addr_str)](auto &&res) {
      if (!res) {
        self->log_->error("cannot close the stream to peer {}, {}: {}", p, a,
                          res.error().message());
      }
    });

    identify::pb::Identify msg;
    msg.ParseFromArray(buffer->data(), read_bytes.value());

    // process a received public key and retrieve an ID of the other peer
    auto received_pubkey_str = msg.has_publickey() ? msg.publickey() : "";
    auto peer_id_opt = consumePublicKey(stream, received_pubkey_str);
    if (!peer_id_opt) {
      // something bad happened during key processing - we can't continue
      return;
    }
    auto peer_id = std::move(*peer_id_opt);

    // store the received protocols
    auto add_res = host_->peerRepository().getProtocolRepository().addProtocols(
        peer_id,
        gsl::make_span(*msg.protocols().pointer_begin(),
                       *msg.protocols().pointer_end()));
    if (!add_res) {
      log_->error("cannot add protocols to peer {}: {}", peer_id.toBase58(),
                  add_res.error().message());
    }

    if (msg.has_observedaddr()) {
      consumeObservedAddresses(msg.observedaddr(), peer_id, stream);
    }

    consumeListenAddresses(gsl::make_span(*msg.listenaddrs().pointer_begin(),
                                          *msg.listenaddrs().pointer_end()),
                           peer_id);
  }

  std::optional<peer::PeerId> Identify::consumePublicKey(
      const StreamSPtr &stream, std::string_view pubkey_str) {
    auto stream_peer_id_res = stream->remotePeerId();

    // if we haven't received a key from the other peer, all we can do is to
    // return the already known peer id
    if (pubkey_str.empty()) {
      if (!stream_peer_id_res) {
        return std::nullopt;
      }
      return stream_peer_id_res.value();
    }

    // peer id can be set in stream, derived from the received public key or
    // both; handle all possible cases
    std::optional<peer::PeerId> stream_peer_id, msg_peer_id;
    std::optional<crypto::PublicKey> pubkey;

    // retrieve a peer id from the stream
    if (stream_peer_id_res) {
      stream_peer_id = std::move(stream_peer_id_res.value());
    }

    // unmarshal a received public key
    auto pubkey_buf = Buffer{}.put(pubkey_str);
    auto pubkey_res = key_marshaller_->unmarshalPublicKey(pubkey_buf);
    if (!pubkey_res) {
      log_->info("cannot unmarshal public key for peer {}: {}",
                 stream_peer_id ? stream_peer_id->toBase58() : "",
                 pubkey_res.error().message());
      return stream_peer_id;
    }
    pubkey = std::move(pubkey_res.value());

    // derive a peer id from the received public key
    auto peer_id_res = peer::PeerId::fromPublicKey(*pubkey);
    if (!peer_id_res) {
      log_->info("cannot create PeerId from the received public key {}: {}",
                 kagome::common::hex_upper(pubkey->data),
                 peer_id_res.error().message());
      return stream_peer_id;
    }
    msg_peer_id = std::move(peer_id_res.value());

    auto &key_repo = host_->peerRepository().getKeyRepository();
    if (!stream_peer_id) {
      // didn't know the ID before; memorize the key, from which it can be
      // derived later
      key_repo.addPublicKey(*msg_peer_id, *pubkey);
      return msg_peer_id;
    }

    if (stream_peer_id && *stream_peer_id != *msg_peer_id) {
      log_->error(
          "peer with id {} sent public key, which derives to id {}, but they "
          "must be equal",
          stream_peer_id->toBase58(), msg_peer_id->toBase58());
      return std::nullopt;
    }

    // insert the derived key into key repository
    key_repo.addPublicKey(*stream_peer_id, *pubkey);
  }

  void Identify::consumeObservedAddresses(const std::string &address_str,
                                          const peer::PeerId &peer_id,
                                          const StreamSPtr &stream) {
    // in order for observed addresses feature to work, all those parameters
    // must be gotten
    auto remote_addr_res = stream->remoteMultiaddr();
    auto local_addr_res = stream->localMultiaddr();
    auto is_initiator_res = stream->isInitiator();
    if (!remote_addr_res || !local_addr_res || !is_initiator_res) {
      return;
    }

    auto address_res = multi::Multiaddress::create(address_str);
    if (!address_res) {
      return log_->error("peer {} has send an invalid observed address",
                         peer_id.toBase58());
    }
    auto &&observed_address = address_res.value();

    // if our local address is not one of our "official" listen addresses, we
    // are not going to save its mapping to the observed one
    // TODO(akvinikym): also getInterfaceListenAddresses() when added
    auto listen_addresses = host_->network().getListenAddresses();
    if (std::find(listen_addresses.begin(), listen_addresses.end(),
                  observed_address)
        == listen_addresses.end()) {
      return;
    }

    // TODO(akvinikym): hasConsistentTransport(..)

    observed_addresses_.add(
        std::move(observed_address), std::move(local_addr_res.value()),
        std::move(remote_addr_res.value()), is_initiator_res.value());
  }

  void Identify::consumeListenAddresses(
      gsl::span<const std::string> addresses_strings,
      const peer::PeerId &peer_id) {
    if (addresses_strings.empty()) {
      return;
    }

    std::vector<multi::Multiaddress> listen_addresses;
    for (const auto &addr_str : addresses_strings) {
      auto addr_res = multi::Multiaddress::create(addr_str);
      if (!addr_res) {
        log_->error("peer {} has sent an invalid listen address",
                    peer_id.toBase58());
        continue;
      }
      listen_addresses.push_back(std::move(addr_res.value()));
    }

    auto &addr_repo = host_->peerRepository().getAddressRepository();

    // invalidate previously known addresses of that peer
    auto add_res = addr_repo.updateAddresses(peer_id, peer::ttl::kTransient);
    if (!add_res) {
      log_->error("cannot update listen addresses of the peer {}: {}",
                  peer_id.toBase58(), add_res.error().message());
    }

    // memorize the addresses
    switch (host_->network().connectedness(peer_id)) {
      case network::Network::Connectedness::CONNECTED:
        add_res = addr_repo.upsertAddresses(peer_id, listen_addresses,
                                            peer::ttl::kPermanent);
        break;
      default:
        add_res = addr_repo.upsertAddresses(peer_id, listen_addresses,
                                            peer::ttl::kRecentlyConnected);
        break;
    }
    if (!add_res) {
      log_->error("cannot add addresses to peer {}: {}", peer_id.toBase58(),
                  add_res.error().message());
    }
  }
}  // namespace libp2p::protocol
