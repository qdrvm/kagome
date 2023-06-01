/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "authority_discovery/query/query_impl.hpp"

#include "authority_discovery/protobuf/authority_discovery.v2.pb.h"
#include "common/bytestr.hpp"
#include "crypto/sha/sha256.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::authority_discovery, QueryImpl::Error, e) {
  using E = decltype(e);
  switch (e) {
    case E::DECODE_ERROR:
      return "Decode error";
    case E::NO_ADDRESSES:
      return "No addresses";
    case E::INCONSISTENT_PEER_ID:
      return "Inconsistent peer id";
    case E::INVALID_SIGNATURE:
      return "Invalid signature";
  }
  return fmt::format("authority_discovery::QueryImpl::Error({})", e);
}

namespace kagome::authority_discovery {
  constexpr size_t kMaxActiveRequests = 8;
  constexpr std::chrono::seconds kIntervalInitial{2};
  constexpr std::chrono::minutes kIntervalMax{10};

  QueryImpl::QueryImpl(
      std::shared_ptr<application::AppStateManager> app_state_manager,
      std::shared_ptr<blockchain::BlockTree> block_tree,
      std::shared_ptr<runtime::AuthorityDiscoveryApi> authority_discovery_api,
      std::shared_ptr<crypto::CryptoStore> crypto_store,
      std::shared_ptr<crypto::Sr25519Provider> sr_crypto_provider,
      std::shared_ptr<libp2p::crypto::CryptoProvider> libp2p_crypto_provider,
      std::shared_ptr<libp2p::crypto::marshaller::KeyMarshaller> key_marshaller,
      libp2p::Host &host,
      std::shared_ptr<libp2p::protocol::kademlia::Kademlia> kademlia,
      std::shared_ptr<libp2p::basic::Scheduler> scheduler)
      : block_tree_{std::move(block_tree)},
        authority_discovery_api_{std::move(authority_discovery_api)},
        crypto_store_{std::move(crypto_store)},
        sr_crypto_provider_{std::move(sr_crypto_provider)},
        libp2p_crypto_provider_{std::move(libp2p_crypto_provider)},
        key_marshaller_{std::move(key_marshaller)},
        host_{host},
        kademlia_{std::move(kademlia)},
        scheduler_{[&] {
          BOOST_ASSERT(scheduler != nullptr);
          return std::move(scheduler);
        }()},
        interval_{
            kIntervalInitial,
            kIntervalMax,
            scheduler_,
        },
        log_{log::createLogger("AuthorityDiscoveryQuery",
                               "authority_discovery")} {
    app_state_manager->takeControl(*this);
  }

  bool QueryImpl::start() {
    interval_.start([this] {
      auto r = update();
      if (not r) {
        SL_WARN(log_, "update: {}", r.error());
      }
    });
    return true;
  }

  std::optional<libp2p::peer::PeerInfo> QueryImpl::get(
      const primitives::AuthorityDiscoveryId &authority) const {
    std::unique_lock lock{mutex_};
    auto it = auth_to_peer_cache_.find(authority);
    if (it != auth_to_peer_cache_.end()) {
      return it->second;
    }
    return std::nullopt;
  }

  std::optional<primitives::AuthorityDiscoveryId> QueryImpl::get(
      const libp2p::peer::PeerId &peer_id) const {
    std::unique_lock lock{mutex_};
    auto it = peer_to_auth_cache_.find(peer_id);
    if (it != peer_to_auth_cache_.end()) {
      return it->second;
    }
    return std::nullopt;
  }

  outcome::result<void> QueryImpl::update() {
    std::unique_lock lock{mutex_};
    OUTCOME_TRY(
        authorities,
        authority_discovery_api_->authorities(block_tree_->bestLeaf().hash));
    OUTCOME_TRY(local_keys,
                crypto_store_->getSr25519PublicKeys(
                    crypto::KnownKeyTypeId::KEY_TYPE_AUDI));
    authorities.erase(
        std::remove_if(authorities.begin(),
                       authorities.end(),
                       [&](const primitives::AuthorityDiscoveryId &id) {
                         return std::find(
                                    local_keys.begin(), local_keys.end(), id)
                             != local_keys.end();
                       }),
        authorities.end());
    for (auto it = auth_to_peer_cache_.begin();
         it != auth_to_peer_cache_.end();) {
      if (std::find(authorities.begin(), authorities.end(), it->first)
          != authorities.end()) {
        ++it;
      } else {
        it = auth_to_peer_cache_.erase(it);
      }
    }
    for (auto it = peer_to_auth_cache_.begin();
         it != peer_to_auth_cache_.end();) {
      if (std::find(authorities.begin(), authorities.end(), it->second)
          != authorities.end()) {
        ++it;
      } else {
        it = peer_to_auth_cache_.erase(it);
      }
    }
    std::shuffle(authorities.begin(), authorities.end(), random_);
    queue_ = std::move(authorities);
    pop();
    return outcome::success();
  }

  void QueryImpl::pop() {
    while (active_ < kMaxActiveRequests) {
      if (queue_.empty()) {
        return;
      }
      ++active_;
      auto authority = queue_.back();
      queue_.pop_back();

      common::Buffer hash = crypto::sha256(authority);
      scheduler_->schedule([=, wp = weak_from_this()] {
        if (auto self = wp.lock()) {
          std::ignore = kademlia_->getValue(
              hash, [=](outcome::result<std::vector<uint8_t>> res) {
                std::unique_lock lock{mutex_};
                --active_;
                pop();
                if (res.has_error()) {
                  SL_WARN(log_, "Kademlia can't get value: {}", res.error());
                  return;
                }
                auto r = add(authority, std::move(res.value()));
                if (not r) {
                  SL_WARN(log_, "Can't add: {}", r.error());
                }
              });
        }
      });
    }
  }

  outcome::result<void> QueryImpl::add(
      const primitives::AuthorityDiscoveryId &authority,
      outcome::result<std::vector<uint8_t>> _res) {
    OUTCOME_TRY(signed_record_pb, _res);
    ::authority_discovery::v2::SignedAuthorityRecord signed_record;
    if (not signed_record.ParseFromArray(signed_record_pb.data(),
                                         signed_record_pb.size())) {
      return Error::DECODE_ERROR;
    }
    libp2p::crypto::ProtobufKey protobuf_key{
        common::Buffer{str2byte(signed_record.peer_signature().public_key())}};
    OUTCOME_TRY(peer_key, key_marshaller_->unmarshalPublicKey(protobuf_key));
    OUTCOME_TRY(peer_id, libp2p::peer::PeerId::fromPublicKey(protobuf_key));
    if (peer_id == host_.getId()) {
      return outcome::success();
    }

    OUTCOME_TRY(auth_sig,
                crypto::Sr25519Signature::fromSpan(
                    str2byte(signed_record.auth_signature())));

    ::authority_discovery::v2::AuthorityRecord record;
    if (not record.ParseFromString(signed_record.record())) {
      return Error::DECODE_ERROR;
    }
    if (record.addresses().empty()) {
      return Error::NO_ADDRESSES;
    }
    libp2p::peer::PeerInfo peer{std::move(peer_id), {}};
    auto peer_id_str = peer.id.toBase58();
    for (auto &pb : record.addresses()) {
      OUTCOME_TRY(address, libp2p::multi::Multiaddress::create(str2byte(pb)));
      auto id = address.getPeerId();
      if (not id) {
        continue;
      }
      if (id != peer_id_str) {
        return Error::INCONSISTENT_PEER_ID;
      }
      peer.addresses.emplace_back(std::move(address));
    }

    OUTCOME_TRY(auth_sig_ok,
                sr_crypto_provider_->verify(
                    auth_sig, str2byte(signed_record.record()), authority));
    if (not auth_sig_ok) {
      return Error::INVALID_SIGNATURE;
    }

    OUTCOME_TRY(peer_sig_ok,
                libp2p_crypto_provider_->verify(
                    str2byte(signed_record.record()),
                    str2byte(signed_record.peer_signature().signature()),
                    peer_key));
    if (not peer_sig_ok) {
      return Error::INVALID_SIGNATURE;
    }

    peer_to_auth_cache_.insert_or_assign(peer.id, authority);
    auth_to_peer_cache_.insert_or_assign(authority, std::move(peer));

    return outcome::success();
  }
}  // namespace kagome::authority_discovery
