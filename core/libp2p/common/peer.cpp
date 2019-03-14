/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/common/peer.hpp"

#include <boost/format.hpp>
#include "libp2p/multi/multibase_codec/multibase_codec_impl.hpp"

namespace libp2p::common {
  using namespace kagome::expected;
  using Encodings = multi::MultibaseCodec::Encoding;

  Peer::Peer(const kagome::common::Buffer &id)
      : id_{id}, base_codec_{std::make_unique<multi::MultibaseCodecImpl>()} {}

  Peer::Peer(const kagome::common::Buffer &id, const security::Key &public_key,
             const security::Key &private_key)
      : id_{id},
        public_key_{public_key},
        private_key_{private_key},
        base_codec_{std::make_unique<multi::MultibaseCodecImpl>()} {}

  Peer::Peer::FactoryResult Peer::createPeer(const kagome::common::Buffer &id,
                                             const security::Key &public_key,
                                             const security::Key &private_key) {
    if (id.size() == 0) {
      return Error{"can not construct Peer with empty id"};
    }
    /// check that private_key => public_key

    auto peer = Peer{id, public_key, private_key};
    return Value{std::move(peer)};
  }

  Peer::FactoryResult Peer::createPeer(
      const kagome::common::Buffer &public_key) {}

  //    Peer::FactoryResult Peer::createPeer(const kagome::common::Buffer
  //    &private_key) { }

  Peer::FactoryResult Peer::createFromHexString(std::string_view id) {}

  Peer::FactoryResult Peer::createFromBytes(const kagome::common::Buffer &id) {}

  Peer::FactoryResult Peer::createFromB58String(std::string_view id) {}

  Peer::FactoryResult Peer::createFromPublicKey(
      const security::Key &public_key) {}

  Peer::FactoryResult Peer::createFromPrivateKey(
      const security::Key &private_key) {}

  std::string_view Peer::toHex() const {
    return base_codec_->encode(id_, Encodings::kBase16Lower);
  }

  const kagome::common::Buffer &Peer::toBytes() const {
    return id_;
  }

  std::string_view Peer::toBase58() const {
    return base_codec_->encode(id_, Encodings::kBase58);
  }

  boost::optional<security::Key> Peer::publicKey() const {
    return public_key_;
  }

  boost::optional<security::Key> Peer::privateKey() const {
    return private_key_;
  }

  const PeerInfo &Peer::peerInfo() const {
    return peer_info_;
  }

  std::string_view Peer::toString() const {
    return (boost::format("Peer: {id = %s, pubkey = %s, privkey = %s}")
            % toHex() % (public_key_ ? public_key_->toString() : "")
            % (private_key_ ? private_key_->toString() : ""))
        .str();
  }

  bool Peer::operator==(const Peer &other) const {
    return this->id_ == other.id_ && this->private_key_ == other.private_key_
        && this->public_key_ == other.public_key_
        && this->peer_info_ == other.peer_info_;
  }

}  // namespace libp2p::common
