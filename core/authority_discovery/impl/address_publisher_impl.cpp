/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "authority_discovery/impl/address_publisher_impl.hpp"

#include "authority_discovery/impl/address_publisher_errors.hpp"
#include "authority_discovery/protobuf/authority_discovery.v2.pb.h"
#include "authority_discovery/protobuf/keys.pb.h"

#include <libp2p/multi/multibase_codec/codecs/base58.hpp>
#include <utility>

#include "crypto/sha/sha256.hpp"

namespace kagome::authority_discovery {
  void AddressPublisherImpl::run() {
    // TODO(ortyomka): run in scheduler with some interval
    auto maybe_error = publishOwnAddress();
    if (maybe_error.has_error()) {
      SL_ERROR(
          log_, "publishOwnAddress failed: {}", maybe_error.error().message());
    }
  }

  outcome::result<void> AddressPublisherImpl::publishOwnAddress() {
    using libp2p::protocol::kademlia::Key;
    using libp2p::protocol::kademlia::Value;

    auto gran_key = keys_->getGranKeyPair();
    if (not gran_key) {
      return AddressPublisherError::NO_GRAND_KEY;
    }

    auto best_hash = block_tree_->deepestLeaf();
    auto authority_opt = authority_manager_->authorities(best_hash, false);
    if (not authority_opt.has_value()) {
      return AddressPublisherError::NO_AUTHORITY;
    }

    auto &authority = authority_opt.value();

    if (std::find_if(authority->begin(),
                     authority->end(),
                     [&gran_key](const auto &elem) {
                       return elem.id.id == gran_key->public_key;
                     })
        == authority->end()) {
      // we are not authority
      return outcome::success();
    }

    auto audi_key = keys_->getAudiKeyPair();
    if (not audi_key) {
      return AddressPublisherError::NO_AUDI_KEY;
    }

    ::authority_discovery::v2::AuthorityRecord addresses;

    for (const auto &address : host_.getPeerInfo().addresses) {
      auto &bytes = address.getBytesAddress();
      addresses.add_addresses(bytes.data(), bytes.size());
    }

    std::vector<uint8_t> encoded_addresses(addresses.ByteSizeLong());
    addresses.SerializeToArray(encoded_addresses.data(),
                               encoded_addresses.size());

    auto node_key = store_->getLibp2pKeypair().value();

    if (node_key.privateKey.type != libp2p::crypto::Key::Type::Ed25519) {
      return AddressPublisherError::WRONG_KEY_TYPE;
    }

    crypto::Ed25519Keypair libp2p_keypair{
        .secret_key =
            crypto::Ed25519PrivateKey::fromSpan(node_key.privateKey.data)
                .value(),
        .public_key =
            crypto::Ed25519PublicKey::fromSpan(node_key.publicKey.data).value(),
    };

    OUTCOME_TRY(signature,
                crypto_provider_->sign(libp2p_keypair, encoded_addresses));

    ::authority_discovery::v2::SignedAuthorityRecord signed_addresses;

    OUTCOME_TRY(auth_signature,
                crypto_provider2_->sign(*audi_key, encoded_addresses));
    signed_addresses.set_auth_signature(auth_signature.data(),
                                        auth_signature.size());

    signed_addresses.set_record(encoded_addresses.data(),
                                encoded_addresses.size());

    ::keys_proto::PublicKey proto_key;

    proto_key.set_type(::keys_proto::KeyType::Ed25519);
    proto_key.set_data(libp2p_keypair.public_key.data(),
                       libp2p_keypair.public_key.size());

    std::vector<uint8_t> encoded_key(proto_key.ByteSizeLong());
    proto_key.SerializeToArray(encoded_key.data(), encoded_key.size());

    auto ps = signed_addresses.mutable_peer_signature();
    ps->set_signature(signature.data(), signature.size());
    ps->set_public_key(encoded_key.data(), encoded_key.size());

    std::vector<uint8_t> value(signed_addresses.ByteSizeLong());
    signed_addresses.SerializeToArray(value.data(), value.size());

    auto hash = crypto::sha256(audi_key->public_key);
    return kademlia_->putValue({hash.begin(), hash.end()}, value);
  }

  AddressPublisherImpl::AddressPublisherImpl(
      std::shared_ptr<authority::AuthorityManager> authority_manager,
      std::shared_ptr<blockchain::BlockTree> block_tree,
      std::shared_ptr<crypto::SessionKeys> keys,
      std::shared_ptr<crypto::CryptoStore> store,
      std::shared_ptr<crypto::Ed25519Provider> crypto_provider,
      std::shared_ptr<crypto::Sr25519Provider> crypto_provider2,
      libp2p::Host &host,
      std::shared_ptr<libp2p::protocol::kademlia::Kademlia> kademlia,
      std::shared_ptr<libp2p::basic::Scheduler> scheduler)
      : authority_manager_(std::move(authority_manager)),
        block_tree_(std::move(block_tree)),
        keys_(std::move(keys)),
        store_(std::move(store)),
        crypto_provider_(std::move(crypto_provider)),
        crypto_provider2_(std::move(crypto_provider2)),
        host_(host),
        kademlia_(std::move(kademlia)),
        scheduler_(std::move(scheduler)),
        log_{log::createLogger("AddressPublisher")} {}
}  // namespace kagome::authority_discovery
