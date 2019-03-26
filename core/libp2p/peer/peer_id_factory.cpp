/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/peer/peer_id_factory.hpp"

#include "common/result.hpp"
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
        == libp2p::multi::HashType::kSha256;
  }
}  // namespace

OUTCOME_REGISTER_CATEGORY(libp2p::peer::PeerIdFactory::FactoryError, e) {
  using FactoryError = libp2p::peer::PeerIdFactory::FactoryError;
  switch (e) {
    case FactoryError::kIdNotSHA256Hash:
      return "provided id is not a SHA-256 multihash";
    case FactoryError::kEmptyIdPassed:
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

  OUTCOME_MAKE_ERROR_CODE(PeerIdFactory::FactoryError)

  PeerIdFactory::PeerIdFactory(const multi::MultibaseCodec &multibase_codec,
                               const crypto::CryptoProvider &crypto_provider)
      : multibase_codec_{multibase_codec}, crypto_provider_{crypto_provider} {}

  PeerIdFactory::FactoryResult PeerIdFactory::createPeerId(
      const kagome::common::Buffer &id) const {
    if (!idIsSha256Multihash(id)) {
      return FactoryError::kIdNotSHA256Hash;
    }
    return PeerId{id, multibase_codec_, crypto_provider_};
  }

  PeerIdFactory::FactoryResult PeerIdFactory::createPeerId(
      kagome::common::Buffer &&id) const {
    if (!idIsSha256Multihash(id)) {
      return FactoryError::kIdNotSHA256Hash;
    }
    return PeerId{std::move(id), multibase_codec_, crypto_provider_};
  }

  PeerIdFactory::FactoryResult PeerIdFactory::createPeerId(
      const kagome::common::Buffer &id,
      std::shared_ptr<crypto::PublicKey> public_key,
      std::shared_ptr<crypto::PrivateKey> private_key) const {
    if (id.size() == 0) {
      return FactoryError::kEmptyIdPassed;
    }
    if (*private_key->publicKey() != *public_key) {
      return FactoryError::kPubkeyIsNotDerivedFromPrivate;
    }
    auto derived_id = PeerId::idFromPublicKey(*public_key, multibase_codec_);
    if (!derived_id || *derived_id != id) {
      return FactoryError::kIdIsNotHashOfPubkey;
    }

    return PeerId{id, std::move(public_key), std::move(private_key),
                  multibase_codec_, crypto_provider_};
  }

  PeerIdFactory::FactoryResult PeerIdFactory::createFromPublicKey(
      std::shared_ptr<crypto::PublicKey> public_key) const {
    auto derived_id = PeerId::idFromPublicKey(*public_key, multibase_codec_);
    if (!derived_id) {
      return FactoryError::kCannotCreateIdFromPubkey;
    }
    PeerId peer{std::move(*derived_id), multibase_codec_, crypto_provider_};
    peer.unsafeSetPublicKey(std::move(public_key));
    return peer;
  }

  PeerIdFactory::FactoryResult PeerIdFactory::createFromPrivateKey(
      std::shared_ptr<crypto::PrivateKey> private_key) const {
    // can't use chaining, because it would require copying the PeerId
    auto peer_res = createFromPublicKey(private_key->publicKey());
    if (peer_res) {
      peer_res.value().unsafeSetPrivateKey(std::move(private_key));
    }
    return peer_res;
  }

  PeerIdFactory::FactoryResult PeerIdFactory::createFromPublicKey(
      const kagome::common::Buffer &public_key) const {
    auto pubkey_ptr = crypto_provider_.unmarshalPublicKey(public_key);
    if (!pubkey_ptr) {
      return FactoryError::kCannotUnmarshalPubkey;
    }
    return createFromPublicKey(std::move(pubkey_ptr));
  }

  PeerIdFactory::FactoryResult PeerIdFactory::createFromPrivateKey(
      const kagome::common::Buffer &private_key) const {
    auto privkey_ptr = crypto_provider_.unmarshalPrivateKey(private_key);
    if (!privkey_ptr) {
      return FactoryError::kCannotUnmarshalPrivkey;
    }
    return createFromPrivateKey(std::move(privkey_ptr));
  }

  PeerIdFactory::FactoryResult PeerIdFactory::createFromPublicKey(
      std::string_view public_key) const {
    auto pubkey_res = multibase_codec_.decode(public_key);
    if (pubkey_res.hasError()) {
      return FactoryError::kCannotDecodePubkey;
    }
    return createFromPublicKey(pubkey_res.getValue());
  }

  PeerIdFactory::FactoryResult PeerIdFactory::createFromPrivateKey(
      std::string_view private_key) const {
    auto privkey_res = multibase_codec_.decode(private_key);
    if (privkey_res.hasError()) {
      return FactoryError::kCannotDecodePrivkey;
    }
    return createFromPrivateKey(privkey_res.getValue());
  }

  PeerIdFactory::FactoryResult PeerIdFactory::createFromEncodedString(
      std::string_view id) const {
    auto id_res = multibase_codec_.decode(id);
    if (id_res.hasError()) {
      return FactoryError::kCannotDecodeId;
    }
    const auto &id_value = id_res.getValueRef();
    if (!idIsSha256Multihash(id_value)) {
      return FactoryError::kIdNotSHA256Hash;
    }

    return PeerId{std::move(id_res.getValue()), multibase_codec_, crypto_provider_};
  }
}  // namespace libp2p::peer
