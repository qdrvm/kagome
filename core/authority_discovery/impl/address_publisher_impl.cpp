/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "authority_discovery/impl/address_publisher_impl.hpp"

#include "authority_discovery/protobuf/authority_discovery.v2.pb.h"
#include "authority_discovery/protobuf/keys.pb.h"

#include <libp2p/multi/multibase_codec/codecs/base58.hpp>
#include <utility>

#include "crypto/sha/sha256.hpp"

namespace kagome::authority_discovery {
  bool AddressPublisherImpl::start() {
    if (not libp2p_key_) {
      return true;
    }
    // TODO(ortyomka): run in scheduler with some interval
    auto maybe_error = publishOwnAddress();
    if (maybe_error.has_error()) {
      SL_ERROR(log_, "publishOwnAddress failed: {}", maybe_error.error());
    }
    return true;
  }

  outcome::result<void> AddressPublisherImpl::publishOwnAddress() {
    using libp2p::protocol::kademlia::Key;
    using libp2p::protocol::kademlia::Value;

    auto audi_key = keys_->getAudiKeyPair();
    if (not audi_key) {
      SL_VERBOSE(log_, "No authority discovery key");
      return outcome::success();
    }

    OUTCOME_TRY(
        authorities,
        authority_discovery_api_->authorities(block_tree_->deepestLeaf().hash));

    if (std::find(authorities.begin(), authorities.end(), audi_key->public_key)
        == authorities.end()) {
      // we are not authority
      return outcome::success();
    }

    ::authority_discovery::v2::AuthorityRecord addresses;

    for (const auto &address : host_.getPeerInfo().addresses) {
      auto &bytes = address.getBytesAddress();
      addresses.add_addresses(bytes.data(), bytes.size());
    }

    std::vector<uint8_t> encoded_addresses(addresses.ByteSizeLong());
    addresses.SerializeToArray(encoded_addresses.data(),
                               encoded_addresses.size());

    OUTCOME_TRY(signature,
                crypto_provider_->sign(*libp2p_key_, encoded_addresses));

    ::authority_discovery::v2::SignedAuthorityRecord signed_addresses;

    OUTCOME_TRY(auth_signature,
                crypto_provider2_->sign(*audi_key, encoded_addresses));
    signed_addresses.set_auth_signature(auth_signature.data(),
                                        auth_signature.size());

    signed_addresses.set_record(encoded_addresses.data(),
                                encoded_addresses.size());

    auto ps = signed_addresses.mutable_peer_signature();
    ps->set_signature(signature.data(), signature.size());
    ps->set_public_key(libp2p_key_pb_->key.data(), libp2p_key_pb_->key.size());

    std::vector<uint8_t> value(signed_addresses.ByteSizeLong());
    signed_addresses.SerializeToArray(value.data(), value.size());

    auto hash = crypto::sha256(audi_key->public_key);
    return kademlia_->putValue({hash.begin(), hash.end()}, value);
  }

  AddressPublisherImpl::AddressPublisherImpl(
      std::shared_ptr<runtime::AuthorityDiscoveryApi> authority_discovery_api,
      std::shared_ptr<application::AppStateManager> app_state_manager,
      std::shared_ptr<blockchain::BlockTree> block_tree,
      std::shared_ptr<crypto::SessionKeys> keys,
      const libp2p::crypto::KeyPair &libp2p_key,
      const libp2p::crypto::marshaller::KeyMarshaller &key_marshaller,
      std::shared_ptr<crypto::Ed25519Provider> crypto_provider,
      std::shared_ptr<crypto::Sr25519Provider> crypto_provider2,
      libp2p::Host &host,
      std::shared_ptr<libp2p::protocol::kademlia::Kademlia> kademlia,
      std::shared_ptr<libp2p::basic::Scheduler> scheduler)
      : authority_discovery_api_(std::move(authority_discovery_api)),
        block_tree_(std::move(block_tree)),
        keys_(std::move(keys)),
        crypto_provider_(std::move(crypto_provider)),
        crypto_provider2_(std::move(crypto_provider2)),
        host_(host),
        kademlia_(std::move(kademlia)),
        scheduler_(std::move(scheduler)),
        log_{log::createLogger("AddressPublisher")} {
    app_state_manager->atLaunch([=] { return start(); });
    if (libp2p_key.privateKey.type == libp2p::crypto::Key::Type::Ed25519) {
      libp2p_key_.emplace(crypto::Ed25519Keypair{
          .secret_key =
              crypto::Ed25519PrivateKey::fromSpan(libp2p_key.privateKey.data)
                  .value(),
          .public_key =
              crypto::Ed25519PublicKey::fromSpan(libp2p_key.publicKey.data)
                  .value(),
      });
      libp2p_key_pb_.emplace(
          key_marshaller.marshal(libp2p_key.publicKey).value());
    } else {
      SL_WARN(log_, "Peer key is not ed25519");
    }
  }
}  // namespace kagome::authority_discovery
