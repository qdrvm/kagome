/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/common/peer.hpp"

#include <boost/format.hpp>
#include "common/hexutil.hpp"
#include "libp2p/multi/multibase_codec/multibase_codec_impl.hpp"
#include "libp2p/multi/multihash.hpp"

namespace {
  /// not a class variable, as static methods are to use it as well
  const libp2p::multi::MultibaseCodecImpl base_codec{};
}  // namespace

namespace libp2p::common {
  using namespace kagome::expected;
  using Encodings = multi::MultibaseCodec::Encoding;

  Peer::Peer(const kagome::common::Buffer &id) : id_{id} {}

  Peer::Peer(kagome::common::Buffer &&id) : id_{std::move(id)} {}

  Peer::Peer(const kagome::common::Buffer &id, const security::Key &public_key,
             const security::Key &private_key)
      : id_{id}, public_key_{public_key}, private_key_{private_key} {}

  Peer::Peer::FactoryResult Peer::createPeer(const kagome::common::Buffer &id,
                                             const security::Key &public_key,
                                             const security::Key &private_key) {
    if (id.size() == 0) {
      return Error{"can not construct Peer with empty id"};
    }
    if (private_key.publicKey() != public_key) {
      return Error{"public key is not derived from the private one"};
    }

    auto peer = Peer{id, public_key, private_key};
    return Value{std::move(peer)};
  }

  Peer::FactoryResult Peer::createPeer(const security::Key &public_key) {
    return multi::Multihash::createFromBuffer(public_key.bytes()) |
        [&public_key](const multi::Multihash &id_multihash) {
          Peer peer{id_multihash.toBuffer()};
          peer.setPublicKey(public_key);
          return peer;
        }
  }

  Peer::FactoryResult Peer::createPeer(const security::Key &private_key) {
    return createPeer(private_key.publicKey()) | [&private_key](Peer &&peer) {
      peer.setPrivateKey(private_key);
      return std::move(peer);
    }
  }

  Peer::FactoryResult Peer::createFromEncodedString(std::string_view id) {
    return base_codec.decode(id) | [](kagome::common::Buffer &&bytes_id) {
      return Value{Peer{std::move(bytes_id)}};
    };
  }

  Peer::FactoryResult Peer::createFromPublicKey(
      const kagome::common::Buffer &public_key) {

  }

  Peer::FactoryResult Peer::createFromPrivateKey(
      const kagome::common::Buffer &private_key) {
    /// don't forget to derive a pubkey!!!
  }

  Peer::FactoryResult Peer::createFromPublicKey(std::string_view public_key) {
    return base_codec.decode(public_key) |
        [](const kagome::common::Buffer &pub_key_bytes) {
          return createFromPublicKey(pub_key_bytes);
        };
  }

  Peer::FactoryResult Peer::createFromPrivateKey(std::string_view private_key) {
    return base_codec.decode(private_key) |
        [](const kagome::common::Buffer &priv_key_bytes) {
          return createFromPrivateKey(priv_key_bytes);
        };
  }

  std::string_view Peer::toHex() const {
    return base_codec.encode(id_, Encodings::kBase16Lower);
  }

  const kagome::common::Buffer &Peer::toBytes() const {
    return id_;
  }

  std::string_view Peer::toBase58() const {
    return base_codec.encode(id_, Encodings::kBase58);
  }

  std::optional<security::Key> Peer::publicKey() const {
    return public_key_;
  }

  bool Peer::setPublicKey(const security::Key &public_key) {
    if (private_key_ && private_key_->publicKey() != public_key) {
      return false;
    }
    public_key_ = public_key;
    return true;
  }

  std::optional<security::Key> Peer::privateKey() const {
    return private_key_;
  }

  bool Peer::setPrivateKey(const security::Key &private_key) {
    const auto &derived_pub_key = private_key.publicKey();
    if (public_key_) {
      if (derived_pub_key != public_key_) {
        return false;
      }
      private_key_ = private_key;
      return true;
    }
    public_key_ = derived_pub_key;
    private_key_ = private_key;
    return true;
  }

  std::optional<kagome::common::Buffer> Peer::marshalPublicKey() const {

    /// REWRITE TO BIND!!!

    if (!public_key_) {
      return {};
    }
    return CryptoProvider::marshal(public_key_);
  }

  std::optional<kagome::common::Buffer> Peer::marshalPrivateKey() const {
    if (!private_key_) {
      return {};
    }
    return CryptoProvider::marshal(private_key_);
  }

  const PeerInfo &Peer::peerInfo() const {
    return peer_info_;
  }

  std::string_view Peer::toString() const {
    return (boost::format("Peer: {id = %s, pubkey = %s, privkey = %s}")
            % toBase58() % (public_key_ ? public_key_->toString() : "")
            % (private_key_ ? private_key_->toString() : ""))
        .str();
  }

  bool Peer::operator==(const Peer &other) const {
    return this->id_ == other.id_ && this->private_key_ == other.private_key_
        && this->public_key_ == other.public_key_
        && this->peer_info_ == other.peer_info_;
  }

}  // namespace libp2p::common
