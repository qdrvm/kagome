/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/peer/peer_id_factory.hpp"

#include "libp2p/multi/multihash.hpp"

namespace libp2p::peer {
  using kagome::expected::Error;
  using kagome::expected::Value;

  PeerIdFactory::PeerIdFactory(const multi::MultibaseCodec &multibase_codec,
                               const crypto::CryptoProvider &crypto_provider)
      : multibase_codec_{multibase_codec}, crypto_provider_{crypto_provider} {}

  PeerId PeerIdFactory::createPeerId(const kagome::common::Buffer &id) {
    return PeerId{id, multibase_codec_, crypto_provider_};
  }

  PeerId PeerIdFactory::createPeerId(kagome::common::Buffer &&id) {
    return PeerId{std::move(id), multibase_codec_, crypto_provider_};
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

    auto peer = PeerId{id, std::move(public_key), std::move(private_key),
                       multibase_codec_, crypto_provider_};
    return Value{std::move(peer)};
  }

  PeerIdFactory::FactoryResult PeerIdFactory::createFromPublicKey(
      std::shared_ptr<crypto::PublicKey> public_key) const {
    return multi::Multihash::createFromBuffer(public_key->getBytes()) |
               [this, public_key = std::move(public_key)](
                   auto &&id_multihash) mutable -> FactoryResult {
      PeerId peer{id_multihash.toBuffer(), multibase_codec_, crypto_provider_};
      peer.setPublicKey(std::move(public_key));
      return Value{std::move(peer)};
    };
  }

  PeerIdFactory::FactoryResult PeerIdFactory::createFromPrivateKey(
      std::shared_ptr<crypto::PrivateKey> private_key) const {
    // cannot reuse 'createFromPublicKey', as a returned PeerId object will be
    // unmodifiable, which does not allows us to set private key as well
    //    auto pubkey_ptr =
    //        std::make_shared<crypto::PublicKey>(private_key->publicKey());
    //    return multi::Multihash::createFromBuffer(pubkey_ptr->getBytes()) |
    //               [this, pubkey_ptr = std::move(pubkey_ptr),
    //                private_key = std::move(private_key)](
    //                   auto &&id_multihash) mutable -> FactoryResult {
    //      PeerId peer{id_multihash.toBuffer(), std::move(pubkey_ptr),
    //                  std::move(private_key), multibase_codec_,
    //                  crypto_provider_};
    //      return Value{std::move(peer)};
    //    };

    return createFromPublicKey(private_key->publicKey()) |
               [private_key = std::move(private_key)](
                   PeerId peer) mutable -> FactoryResult {
      peer.setPrivateKey(std::move(private_key));
      return Value{std::forward<PeerId>(peer)};
    };
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
