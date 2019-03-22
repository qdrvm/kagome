/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/peer/peer_id_factory.hpp"

#include "libp2p/multi/multihash.hpp"

namespace {
  /**
   * Check, if provided buffer is a SHA-256 multihash
   * @param id to be checked
   * @return true, if it is a SHA-256 multihash, false otherwise
   */
  bool idIsSha256Multihash(const kagome::common::Buffer &id) {
    auto multihash_res = libp2p::multi::Multihash::createFromBuffer(id);
    if (multihash_res.hasError()) {
      return false;
    }
    return multihash_res.getValueRef().getType()
        == libp2p::multi::HashType::kSha256;
  }
}  // namespace

namespace libp2p::peer {
  using kagome::expected::Error;
  using kagome::expected::Value;

  PeerIdFactory::PeerIdFactory(const multi::MultibaseCodec &multibase_codec,
                               const crypto::CryptoProvider &crypto_provider)
      : multibase_codec_{multibase_codec}, crypto_provider_{crypto_provider} {}

  PeerIdFactory::FactoryResult PeerIdFactory::createPeerId(
      const kagome::common::Buffer &id) const {
    if (!idIsSha256Multihash(id)) {
      return Error{"provided id is not a SHA-256 multihash"};
    }
    return Value{PeerId{id, multibase_codec_, crypto_provider_}};
  }

  PeerIdFactory::FactoryResult PeerIdFactory::createPeerId(
      kagome::common::Buffer &&id) const {
    if (!idIsSha256Multihash(id)) {
      return Error{"provided id is not a SHA-256 multihash"};
    }
    return Value{PeerId{std::move(id), multibase_codec_, crypto_provider_}};
  }

  PeerIdFactory::FactoryResult PeerIdFactory::createPeerId(
      const kagome::common::Buffer &id,
      std::shared_ptr<crypto::PublicKey> public_key,
      std::shared_ptr<crypto::PrivateKey> private_key) const {
    if (id.size() == 0) {
      return Error{"can not construct Peer with empty id"};
    }
    if (*private_key->publicKey() != *public_key) {
      return Error{"public key is not derived from the private one"};
    }
    auto derived_id = PeerId::idFromPublicKey(*public_key, multibase_codec_);
    if (!derived_id || *derived_id != id) {
      return Error{"id is not a multihash of the public key"};
    }

    return Value{PeerId{id, std::move(public_key), std::move(private_key),
                        multibase_codec_, crypto_provider_}};
  }

  PeerIdFactory::FactoryResult PeerIdFactory::createFromPublicKey(
      std::shared_ptr<crypto::PublicKey> public_key) const {
    auto derived_id = PeerId::idFromPublicKey(*public_key, multibase_codec_);
    if (!derived_id) {
      return Error{"could not create id from the public key"};
    }
    PeerId peer{std::move(*derived_id), multibase_codec_, crypto_provider_};
    peer.unsafeSetPublicKey(std::move(public_key));
    return Value{std::move(peer)};
  }

  PeerIdFactory::FactoryResult PeerIdFactory::createFromPrivateKey(
      std::shared_ptr<crypto::PrivateKey> private_key) const {
    // can't use chaining, because it would require copying the PeerId
    auto peer_res = createFromPublicKey(private_key->publicKey());
    if (peer_res.hasValue()) {
      peer_res.getValueRef().unsafeSetPrivateKey(std::move(private_key));
    }
    return peer_res;
  }

  PeerIdFactory::FactoryResult PeerIdFactory::createFromPublicKey(
      const kagome::common::Buffer &public_key) const {
    auto pubkey_ptr = crypto_provider_.unmarshalPublicKey(public_key);
    if (!pubkey_ptr) {
      return Error{std::string{"could not unmarshal public key from bytes"}};
    }
    return createFromPublicKey(std::move(pubkey_ptr));
  }

  PeerIdFactory::FactoryResult PeerIdFactory::createFromPrivateKey(
      const kagome::common::Buffer &private_key) const {
    auto privkey_ptr = crypto_provider_.unmarshalPrivateKey(private_key);
    if (!privkey_ptr) {
      return Error{"could not unmarshal private key from bytes"};
    }
    return createFromPrivateKey(std::move(privkey_ptr));
  }

  PeerIdFactory::FactoryResult PeerIdFactory::createFromPublicKey(
      std::string_view public_key) const {
    return multibase_codec_.decode(public_key) | [this](auto &&pub_key_bytes) {
      return createFromPublicKey(pub_key_bytes);
    };
  }

  PeerIdFactory::FactoryResult PeerIdFactory::createFromPrivateKey(
      std::string_view private_key) const {
    return multibase_codec_.decode(private_key) |
        [this](auto &&priv_key_bytes) {
          return createFromPrivateKey(priv_key_bytes);
        };
  }

  PeerIdFactory::FactoryResult PeerIdFactory::createFromEncodedString(
      std::string_view id) const {
    return multibase_codec_.decode(id) |
               [this](auto &&bytes_id) -> FactoryResult {
      return Value{PeerId{std::forward<const kagome::common::Buffer>(bytes_id),
                          multibase_codec_, crypto_provider_}};
    };
  }
}  // namespace libp2p::peer
