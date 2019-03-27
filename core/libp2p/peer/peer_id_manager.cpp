/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/peer/peer_id_manager.hpp"

#include "common/result.hpp"
#include "crypto/sha/sha256.hpp"
#include "libp2p/multi/multihash.hpp"

namespace {
  /**
   * Check, if provided buffer is a SHA-256 multihash
   * @param id to be checked
   * @return true, if it is a SHA-256 multihash, false otherwise
   */
  bool idIsSha256Multihash(const kagome::common::Buffer &id) {
    auto multihash_res = libp2p::multi::Multihash::createFromBuffer(id);
    return multihash_res.hasValue()
        && multihash_res.getValueRef().getType()
        == libp2p::multi::HashType::sha256;
  }
}  // namespace

OUTCOME_CPP_DEFINE_CATEGORY(libp2p::peer, PeerIdManager::FactoryError, e) {
  using FactoryError = libp2p::peer::PeerIdManager::FactoryError;
  switch (e) {
    case FactoryError::kIdNotSHA256Hash:
      return "provided id is not a SHA-256 multihash";
    case FactoryError::kEmptyId:
      return "cannot construct Peer with empty id";
    case FactoryError::kPubkeyIsNotDerivedFromPrivate:
      return "public key is not derived from the private one";
    case FactoryError::kIdIsNotHashOfPubkey:
      return "id is not a multihash of the public key";
    case FactoryError::kCannotCreateIdFromPubkey:
      return "cannot create id from the public key";
    case FactoryError::kCannotUnmarshalPubkey:
      return "could not unmarshal public key from bytes";
    case FactoryError::kCannotUnmarshalPrivkey:
      return "could not unmarshal private key from bytes";
    case FactoryError::kCannotDecodePubkey:
      return "cannot decode public key from string";
    case FactoryError::kCannotDecodePrivkey:
      return "cannot decode private key from string";
    case FactoryError::kCannotDecodeId:
      return "cannot decode id from string";
    default:
      return "unknown error";
  }
}

namespace libp2p::peer {
  using kagome::expected::Error;
  using kagome::expected::Value;
  using Encodings = multi::MultibaseCodec::Encoding;

  PeerIdManager::PeerIdManager(const multi::MultibaseCodec &multibase_codec,
                               const crypto::CryptoProvider &crypto_provider)
      : multibase_codec_{multibase_codec}, crypto_provider_{crypto_provider} {}

  PeerIdManager::FactoryResult PeerIdManager::createPeerId(
      const kagome::common::Buffer &id) const {
    if (!idIsSha256Multihash(id)) {
      return FactoryError::kIdNotSHA256Hash;
    }
    return PeerId{id};
  }

  PeerIdManager::FactoryResult PeerIdManager::createPeerId(
      kagome::common::Buffer &&id) const {
    if (!idIsSha256Multihash(id)) {
      return FactoryError::kIdNotSHA256Hash;
    }
    return PeerId{std::move(id)};
  }

  PeerIdManager::FactoryResult PeerIdManager::createPeerId(
      const kagome::common::Buffer &id,
      std::shared_ptr<crypto::PublicKey> public_key,
      std::shared_ptr<crypto::PrivateKey> private_key) const {
    if (id.size() == 0) {
      return FactoryError::kEmptyId;
    }
    if (*private_key->publicKey() != *public_key) {
      return FactoryError::kPubkeyIsNotDerivedFromPrivate;
    }
    auto derived_id = idFromPublicKey(*public_key);
    if (!derived_id || *derived_id != id) {
      return FactoryError::kIdIsNotHashOfPubkey;
    }

    return PeerId{id, std::move(public_key), std::move(private_key)};
  }

  PeerIdManager::FactoryResult PeerIdManager::createFromPublicKey(
      std::shared_ptr<crypto::PublicKey> public_key) const {
    auto derived_id = idFromPublicKey(*public_key);
    if (!derived_id) {
      return FactoryError::kCannotCreateIdFromPubkey;
    }
    PeerId peer{std::move(*derived_id)};
    peer.unsafeSetPublicKey(std::move(public_key));
    return peer;
  }

  PeerIdManager::FactoryResult PeerIdManager::createFromPrivateKey(
      std::shared_ptr<crypto::PrivateKey> private_key) const {
    // can't use chaining, because it would require copying the PeerId
    auto peer_res = createFromPublicKey(private_key->publicKey());
    if (peer_res) {
      peer_res.value().unsafeSetPrivateKey(std::move(private_key));
    }
    return peer_res;
  }

  PeerIdManager::FactoryResult PeerIdManager::createFromPublicKey(
      const kagome::common::Buffer &public_key) const {
    auto pubkey_ptr = crypto_provider_.unmarshalPublicKey(public_key);
    if (!pubkey_ptr) {
      return FactoryError::kCannotUnmarshalPubkey;
    }
    return createFromPublicKey(std::move(pubkey_ptr));
  }

  PeerIdManager::FactoryResult PeerIdManager::createFromPrivateKey(
      const kagome::common::Buffer &private_key) const {
    auto privkey_ptr = crypto_provider_.unmarshalPrivateKey(private_key);
    if (!privkey_ptr) {
      return FactoryError::kCannotUnmarshalPrivkey;
    }
    return createFromPrivateKey(std::move(privkey_ptr));
  }

  PeerIdManager::FactoryResult PeerIdManager::createFromPublicKey(
      std::string_view public_key) const {
    auto pubkey_res = multibase_codec_.decode(public_key);
    if (pubkey_res.hasError()) {
      return FactoryError::kCannotDecodePubkey;
    }
    return createFromPublicKey(pubkey_res.getValue());
  }

  PeerIdManager::FactoryResult PeerIdManager::createFromPrivateKey(
      std::string_view private_key) const {
    auto privkey_res = multibase_codec_.decode(private_key);
    if (privkey_res.hasError()) {
      return FactoryError::kCannotDecodePrivkey;
    }
    return createFromPrivateKey(privkey_res.getValue());
  }

  PeerIdManager::FactoryResult PeerIdManager::createFromEncodedString(
      std::string_view id) const {
    auto id_res = multibase_codec_.decode(id);
    if (id_res.hasError()) {
      return FactoryError::kCannotDecodeId;
    }
    const auto &id_value = id_res.getValueRef();
    if (!idIsSha256Multihash(id_value)) {
      return FactoryError::kIdNotSHA256Hash;
    }

    return PeerId{std::move(id_res.getValue())};
  }

  std::string PeerIdManager::toHex(const PeerId &peer) const {
    return multibase_codec_.encode(peer.id_, Encodings::kBase16Lower);
  }

  std::string PeerIdManager::toBase58(const PeerId &peer) const {
    return multibase_codec_.encode(peer.id_, Encodings::kBase58);
  }

  bool PeerIdManager::setPublicKey(
      PeerId &peer, std::shared_ptr<crypto::PublicKey> public_key) const {
    if (!public_key
        || (peer.private_key_
            && *peer.private_key_->publicKey() != *public_key)) {
      return false;
    }

    if (!idDerivedFromPublicKey(peer, *peer.public_key_)) {
      return false;
    }
    peer.unsafeSetPublicKey(std::move(public_key));
    return true;
  }

  bool PeerIdManager::setPrivateKey(
      PeerId &peer, std::shared_ptr<crypto::PrivateKey> private_key) const {
    if (!private_key) {
      return false;
    }

    auto derived_pub_key = private_key->publicKey();
    if (peer.public_key_) {
      // if public key is set, we need to check, if it's derived from the given
      // private
      if (*derived_pub_key != *peer.public_key_) {
        return false;
      }
    } else {
      // if public key isn't set, we need to check, that id of this peer is
      // derived from the public key, which is derived from the given private
      if (!idDerivedFromPublicKey(peer, *derived_pub_key)) {
        return false;
      }
      peer.unsafeSetPublicKey(std::move(derived_pub_key));
    }
    peer.unsafeSetPrivateKey(std::move(private_key));
    return true;
  }

  std::optional<kagome::common::Buffer> PeerIdManager::marshalPublicKey(
      const PeerId &peer) const {
    if (!peer.public_key_) {
      return {};
    }
    return crypto_provider_.marshal(*peer.public_key_);
  }

  std::optional<kagome::common::Buffer> PeerIdManager::marshalPrivateKey(
      const PeerId &peer) const {
    if (!peer.private_key_) {
      return {};
    }
    return crypto_provider_.marshal(*peer.private_key_);
  }

  bool PeerIdManager::idDerivedFromPublicKey(
      const PeerId &peer, const libp2p::crypto::PublicKey &key) const {
    auto derived_id = idFromPublicKey(key);
    return derived_id && (*derived_id == peer.id_);
  }

  std::optional<kagome::common::Buffer> PeerIdManager::idFromPublicKey(
      const libp2p::crypto::PublicKey &key) const {
    auto encoded_pubkey = multibase_codec_.encode(
        key.getBytes(), multi::MultibaseCodec::Encoding::kBase64);

    // TODO(Akvinikym) PRE-79 21.03.19: substitute crypto::sha256 with only
    // Multihash::create, when hashing will be implemented
    auto multihash_res = multi::Multihash::create(
        multi::HashType::sha256, kagome::crypto::sha256(encoded_pubkey));

    if (multihash_res.hasError()) {
      return {};
    }
    return multihash_res.getValueRef().toBuffer();
  }

  std::string PeerIdManager::toString(const PeerId &peer) const {
    return (boost::format("Peer: {id = %s, pubkey = %s, privkey = %s}")
            % toBase58(peer)
            % (peer.public_key_ ? peer.public_key_->toString() : "")
            % (peer.private_key_ ? peer.private_key_->toString() : ""))
        .str();
  }
}  // namespace libp2p::peer
