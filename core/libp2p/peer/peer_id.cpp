/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/peer/peer_id.hpp"

#include <boost/format.hpp>
#include "libp2p/multi/multihash.hpp"

namespace libp2p::peer {
  using kagome::expected::Error;
  using kagome::expected::Value;
  using Encodings = multi::MultibaseCodec::Encoding;

  PeerId::PeerId(kagome::common::Buffer id,
                 const multi::MultibaseCodec &multibase_codec,
                 const crypto::CryptoProvider &crypto_provider)
      : id_{std::move(id)},
        multibase_codec_{multibase_codec},
        crypto_provider_{crypto_provider} {}

  PeerId::PeerId(kagome::common::Buffer id,
                 std::shared_ptr<crypto::PublicKey> public_key,
                 std::shared_ptr<crypto::PrivateKey> private_key,
                 const multi::MultibaseCodec &multibase_codec,
                 const crypto::CryptoProvider &crypto_provider)
      : id_{std::move(id)},
        public_key_{std::move(public_key)},
        private_key_{std::move(private_key)},
        multibase_codec_{multibase_codec},
        crypto_provider_{crypto_provider} {}

  std::string_view PeerId::toHex() const {
    return multibase_codec_.encode(id_, Encodings::kBase16Lower);
  }

  const kagome::common::Buffer &PeerId::toBytes() const {
    return id_;
  }

  std::string_view PeerId::toBase58() const {
    return multibase_codec_.encode(id_, Encodings::kBase58);
  }

  std::shared_ptr<crypto::PublicKey> PeerId::publicKey() const {
    return public_key_;
  }

  bool PeerId::setPublicKey(std::shared_ptr<crypto::PublicKey> public_key) {
    if (!public_key
        || (private_key_ && *private_key_->publicKey() != *public_key)) {
      return false;
    }
    public_key_ = std::move(public_key);
    return true;
  }

  std::shared_ptr<crypto::PrivateKey> PeerId::privateKey() const {
    return private_key_;
  }

  bool PeerId::setPrivateKey(std::shared_ptr<crypto::PrivateKey> private_key) {
    if (!private_key) {
      return false;
    }

    auto derived_pub_key = private_key->publicKey();
    if (public_key_) {
      if (*derived_pub_key != *public_key_) {
        return false;
      }
    } else {
      public_key_ = std::move(derived_pub_key);
    }
    private_key_ = std::move(private_key);
    return true;
  }

  std::optional<kagome::common::Buffer> PeerId::marshalPublicKey() const {
    if (!public_key_) {
      return {};
    }
    return crypto_provider_.marshal(*public_key_);
  }

  std::optional<kagome::common::Buffer> PeerId::marshalPrivateKey() const {
    if (!private_key_) {
      return {};
    }
    return crypto_provider_.marshal(*private_key_);
  }

  std::string_view PeerId::toString() const {
    return (boost::format("Peer: {id = %s, pubkey = %s, privkey = %s}")
            % toBase58() % (public_key_ ? public_key_->toString() : "")
            % (private_key_ ? private_key_->toString() : ""))
        .str();
  }

  bool PeerId::operator==(const PeerId &other) const {
    return this->id_ == other.id_ && this->private_key_ == other.private_key_
        && this->public_key_ == other.public_key_;
  }

  void PeerId::unsafeSetPublicKey(
      std::shared_ptr<crypto::PublicKey> public_key) {
    public_key_ = std::move(public_key);
  }

  void PeerId::unsafeSetPrivateKey(
      std::shared_ptr<crypto::PrivateKey> private_key) {
    private_key_ = std::move(private_key);
  }
}  // namespace libp2p::peer
