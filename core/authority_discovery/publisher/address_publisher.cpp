/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "authority_discovery/publisher/address_publisher.hpp"

#include "authority_discovery/protobuf/authority_discovery.v2.pb.h"

#include "authority_discovery/timestamp.hpp"
#include "crypto/sha/sha256.hpp"

#define _PB_SPAN(f) \
  [&](::kagome::common::BufferView a) { (f)(a.data(), a.size()); }
#define PB_SPAN_SET(a, b, c) _PB_SPAN((a).set_##b)(c)
#define PB_SPAN_ADD(a, b, c) _PB_SPAN((a).add_##b)(c)

template <typename T>
std::vector<uint8_t> pbEncodeVec(const T &v) {
  std::vector<uint8_t> r(v.ByteSizeLong());
  v.SerializeToArray(r.data(), r.size());
  return r;
}

namespace kagome::authority_discovery {
  constexpr std::chrono::seconds kIntervalInitial{2};
  constexpr std::chrono::hours kIntervalMax{1};

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
        interval_{kIntervalInitial, kIntervalMax, std::move(scheduler)},
        log_{log::createLogger("AddressPublisher", "authority_discovery")} {
    BOOST_ASSERT(authority_discovery_api_ != nullptr);
    BOOST_ASSERT(app_state_manager != nullptr);
    BOOST_ASSERT(block_tree_ != nullptr);
    BOOST_ASSERT(keys_ != nullptr);
    BOOST_ASSERT(ed_crypto_provider_ != nullptr);
    BOOST_ASSERT(sr_crypto_provider_ != nullptr);
    BOOST_ASSERT(kademlia_ != nullptr);
    app_state_manager->atLaunch([this] { return start(); });
    if (libp2p_key.privateKey.type == libp2p::crypto::Key::Type::Ed25519) {
      std::array<uint8_t, crypto::constants::ed25519::PRIVKEY_SIZE>
          private_key_bytes{};
      std::copy_n(libp2p_key.privateKey.data.begin(),
                  crypto::constants::ed25519::PRIVKEY_SIZE,
                  private_key_bytes.begin());
      libp2p_key_.emplace(crypto::Ed25519Keypair{
          .secret_key = crypto::Ed25519PrivateKey::from(
              crypto::SecureCleanGuard(private_key_bytes)),
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
    if (not roles_.isAuthority()) {
      return true;
    }
    interval_.start([weak_self{weak_from_this()}] {
      auto self = weak_self.lock();
      if (not self) {
        return;
      }
      auto maybe_error = self->publishOwnAddress();
      if (not maybe_error) {
        SL_WARN(
            self->log_, "publishOwnAddress failed: {}", maybe_error.error());
      }
    });
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
    // if (not audi_key) {
    //   SL_WARN(log_, "No authority discovery key");
    //   return outcome::success();
    // }

    OUTCOME_TRY(
        raw,
        audiEncode(ed_crypto_provider_,
                   sr_crypto_provider_,
                   *libp2p_key_,
                   *libp2p_key_pb_,
                   peer_info,
                   *audi_key,
                   std::chrono::system_clock::now().time_since_epoch()));
    return kademlia_->putValue(std::move(raw.first), std::move(raw.second));
  }

  outcome::result<std::pair<Buffer, Buffer>> audiEncode(
      std::shared_ptr<crypto::Ed25519Provider> ed_crypto_provider,
      std::shared_ptr<crypto::Sr25519Provider> sr_crypto_provider,
      const Ed25519Keypair &libp2p_key,
      const ProtobufKey &libp2p_key_pb,
      const PeerInfo &peer_info,
      const Sr25519Keypair &audi_key,
      std::optional<std::chrono::nanoseconds> now) {
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
    ::authority_discovery_v2::AuthorityRecord record;
    for (const auto &address : addresses) {
      PB_SPAN_ADD(record, addresses, address.getBytesAddress());
    }
    // if (now) {
    //   TimestampScale time{now->count()};
    //   OUTCOME_TRY(encoded_time, scale::encode(time));
    //   PB_SPAN_SET(*record.mutable_creation_time(), timestamp, encoded_time);
    // }

    auto record_pb = pbEncodeVec(record);
    OUTCOME_TRY(signature, ed_crypto_provider->sign(libp2p_key, record_pb));
    OUTCOME_TRY(auth_signature, sr_crypto_provider->sign(audi_key, record_pb));

    ::authority_discovery_v2::SignedAuthorityRecord signed_record;
    PB_SPAN_SET(signed_record, auth_signature, auth_signature);
    PB_SPAN_SET(signed_record, record, record_pb);
    auto &ps = *signed_record.mutable_peer_signature();
    PB_SPAN_SET(ps, signature, signature);
    PB_SPAN_SET(ps, public_key, libp2p_key_pb.key);

    auto hash = crypto::sha256(audi_key.public_key);
    return std::make_pair(Buffer{hash}, Buffer{pbEncodeVec(signed_record)});
  }
}  // namespace kagome::authority_discovery
