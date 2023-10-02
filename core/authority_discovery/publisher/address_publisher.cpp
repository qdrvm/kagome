/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "authority_discovery/publisher/address_publisher.hpp"

#include "authority_discovery/protobuf/authority_discovery.v2.pb.h"

#include "crypto/sha/sha256.hpp"

#define _PB_SPAN(f) [&](gsl::span<const uint8_t> a) { (f)(a.data(), a.size()); }
#define PB_SPAN_SET(a, b, c) _PB_SPAN((a).set_##b)(c)
#define PB_SPAN_ADD(a, b, c) _PB_SPAN((a).add_##b)(c)

template <typename T>
std::vector<uint8_t> pbEncodeVec(const T &v) {
  std::vector<uint8_t> r(v.ByteSizeLong());
  v.SerializeToArray(r.data(), r.size());
  return r;
}

namespace kagome::authority_discovery {
  AddressPublisher::AddressPublisher(
      std::shared_ptr<runtime::AuthorityDiscoveryApi> authority_discovery_api,
      network::Roles roles,
      std::shared_ptr<application::AppStateManager> app_state_manager,
      std::shared_ptr<blockchain::BlockTree> block_tree,
      std::shared_ptr<crypto::SessionKeys> keys,
      const libp2p::crypto::KeyPair &libp2p_key,
      const libp2p::crypto::marshaller::KeyMarshaller &key_marshaller,
      std::shared_ptr<crypto::Ed25519Provider> ed_crypto_provider,
      std::shared_ptr<crypto::Sr25519Provider> sr_crypto_provider,
      libp2p::Host &host,
      std::shared_ptr<libp2p::protocol::kademlia::Kademlia> kademlia,
      std::shared_ptr<libp2p::basic::Scheduler> scheduler)
      : authority_discovery_api_(std::move(authority_discovery_api)),
        roles_(roles),
        block_tree_(std::move(block_tree)),
        keys_(std::move(keys)),
        ed_crypto_provider_(std::move(ed_crypto_provider)),
        sr_crypto_provider_(std::move(sr_crypto_provider)),
        host_(host),
        kademlia_(std::move(kademlia)),
        scheduler_(std::move(scheduler)),
        log_{log::createLogger("AddressPublisher", "authority_discovery")} {
    BOOST_ASSERT(authority_discovery_api_ != nullptr);
    BOOST_ASSERT(app_state_manager != nullptr);
    BOOST_ASSERT(block_tree_ != nullptr);
    BOOST_ASSERT(keys_ != nullptr);
    BOOST_ASSERT(ed_crypto_provider_ != nullptr);
    BOOST_ASSERT(sr_crypto_provider_ != nullptr);
    BOOST_ASSERT(kademlia_ != nullptr);
    BOOST_ASSERT(scheduler_ != nullptr);
    app_state_manager->atLaunch([this] { return start(); });
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

  bool AddressPublisher::start() {
    if (not libp2p_key_) {
      return true;
    }
    if (not roles_.flags.authority) {
      return true;
    }
    // TODO(ortyomka): #1358, republish in scheduler with some interval
    auto maybe_error = publishOwnAddress();
    if (maybe_error.has_error()) {
      SL_ERROR(log_, "publishOwnAddress failed: {}", maybe_error.error());
    }
    return true;
  }

  outcome::result<void> AddressPublisher::publishOwnAddress() {
    const auto peer_info = host_.getPeerInfo();
    // TODO(turuslan): #1357, filter local addresses
    if (peer_info.addresses.empty()) {
      SL_ERROR(log_, "No listening addresses");
      return outcome::success();
    }

    OUTCOME_TRY(
        authorities,
        authority_discovery_api_->authorities(block_tree_->bestBlock().hash));

    auto audi_key = keys_->getAudiKeyPair(authorities);
    if (not audi_key) {
      SL_WARN(log_, "No authority discovery key");
      return outcome::success();
    }

    std::unordered_set<libp2p::multi::Multiaddress> addresses;
    for (const auto &address : peer_info.addresses) {
      if (address.getPeerId()) {
        addresses.emplace(address);
        continue;
      }
      std::string s{address.getStringAddress()};
      s.append("/p2p/");
      s.append(peer_info.id.toBase58());
      OUTCOME_TRY(address2, libp2p::multi::Multiaddress::create(s));
      addresses.emplace(std::move(address2));
    }
    ::authority_discovery::v2::AuthorityRecord record;
    for (const auto &address : addresses) {
      PB_SPAN_ADD(record, addresses, address.getBytesAddress());
    }

    auto record_pb = pbEncodeVec(record);
    OUTCOME_TRY(signature, ed_crypto_provider_->sign(*libp2p_key_, record_pb));
    OUTCOME_TRY(auth_signature,
                sr_crypto_provider_->sign(*audi_key, record_pb));

    ::authority_discovery::v2::SignedAuthorityRecord signed_record;
    PB_SPAN_SET(signed_record, auth_signature, auth_signature);
    PB_SPAN_SET(signed_record, record, record_pb);
    auto &ps = *signed_record.mutable_peer_signature();
    PB_SPAN_SET(ps, signature, signature);
    PB_SPAN_SET(ps, public_key, libp2p_key_pb_->key);

    auto hash = crypto::sha256(audi_key->public_key);
    return kademlia_->putValue({hash.begin(), hash.end()},
                               pbEncodeVec(signed_record));
  }
}  // namespace kagome::authority_discovery
