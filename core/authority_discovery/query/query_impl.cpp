/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "authority_discovery/query/query_impl.hpp"

#include "authority_discovery/protobuf/authority_discovery.v2.pb.h"
#include "common/buffer_view.hpp"
#include "common/bytestr.hpp"
#include "crypto/sha/sha256.hpp"
#include "network/impl/protocols/parachain.hpp"
#include "utils/retain_if.hpp"

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
    case E::KADEMLIA_OUTDATED_VALUE:
      return "Kademlia outdated value";
  }
  return "unknown error (authority_discovery::QueryImpl::Error)";
}

namespace kagome::authority_discovery {
  constexpr size_t kMaxActiveRequests = 8;
  constexpr std::chrono::seconds kIntervalInitial{2};
  constexpr std::chrono::minutes kIntervalMax{10};

  QueryImpl::QueryImpl(
      std::shared_ptr<application::AppStateManager> app_state_manager,
      std::shared_ptr<blockchain::BlockTree> block_tree,
      std::shared_ptr<runtime::AuthorityDiscoveryApi> authority_discovery_api,
      LazySPtr<network::ValidationProtocolReserve> validation_protocol,
      std::shared_ptr<crypto::KeyStore> key_store,
      std::shared_ptr<AudiStore> audi_store,
      std::shared_ptr<crypto::Sr25519Provider> sr_crypto_provider,
      std::shared_ptr<libp2p::crypto::CryptoProvider> libp2p_crypto_provider,
      std::shared_ptr<libp2p::crypto::marshaller::KeyMarshaller> key_marshaller,
      libp2p::Host &host,
      LazySPtr<libp2p::protocol::kademlia::Kademlia> kademlia,
      std::shared_ptr<libp2p::basic::Scheduler> scheduler)
      : block_tree_{std::move(block_tree)},
        authority_discovery_api_{std::move(authority_discovery_api)},
        validation_protocol_{std::move(validation_protocol)},
        key_store_{std::move(key_store)},
        audi_store_(std::move(audi_store)),
        sr_crypto_provider_{std::move(sr_crypto_provider)},
        libp2p_crypto_provider_{std::move(libp2p_crypto_provider)},
        key_marshaller_{std::move(key_marshaller)},
        host_{host},
        kademlia_{kademlia},
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
    BOOST_ASSERT(audi_store_ != nullptr);
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
    auto authority_opt = audi_store_->get(authority);
    if (authority_opt == std::nullopt) {
      SL_TRACE(log_,
               "No authority peer found in storage {}",
               common::hex_lower(authority));
      return std::nullopt;
    }
    SL_TRACE(log_,
             "Authority id {} {} addresses found in storage",
             common::hex_lower(authority),
             authority_opt.value().peer.addresses.size());
    return authority_opt.value().peer;
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

  outcome::result<void> QueryImpl::validate(
      const libp2p::protocol::kademlia::Key &key,
      const libp2p::protocol::kademlia::Value &value) {
    std::unique_lock lock{mutex_};
    auto id = hashToAuth(key);
    if (not id) {
      lock.unlock();
      return kademlia_validator_.validate(key, value);
    }
    auto r = add(*id, value);
    if (not r) {
      SL_DEBUG(log_, "Can't add: {}", r.error());
    }
    return r;
  }

  outcome::result<size_t> QueryImpl::select(
      const libp2p::protocol::kademlia::Key &key,
      const std::vector<libp2p::protocol::kademlia::Value> &values) {
    std::unique_lock lock{mutex_};
    auto id = hashToAuth(key);
    if (not id) {
      lock.unlock();
      return kademlia_validator_.select(key, values);
    }
    auto authority = audi_store_->get(*id);
    if (authority != std::nullopt) {
      auto it_value = std::ranges::find(values, authority.value().raw);
      if (it_value != values.end()) {
        return it_value - values.begin();
      }
    }
    return Error::KADEMLIA_OUTDATED_VALUE;
  }

  outcome::result<void> QueryImpl::update() {
    std::unique_lock lock{mutex_};
    OUTCOME_TRY(
        authorities,
        authority_discovery_api_->authorities(block_tree_->bestBlock().hash));
    for (auto &id : authorities) {
      if (not hash_to_auth_.contains(id)) {
        hash_to_auth_.emplace(crypto::sha256(id), id);
      }
    }
    OUTCOME_TRY(local_keys,
                key_store_->sr25519().getPublicKeys(
                    crypto::KeyTypes::AUTHORITY_DISCOVERY));
    auto has = [](const std::vector<primitives::AuthorityDiscoveryId> &keys,
                  const primitives::AuthorityDiscoveryId &key) {
      return std::ranges::find(keys, key) != keys.end();
    };
    retain_if(authorities, [&](const primitives::AuthorityDiscoveryId &id) {
      return not has(local_keys, id);
    });
    // remove outdated authorities
    audi_store_->retainIf([&](const primitives::AuthorityDiscoveryId &id,
                              const AuthorityPeerInfo &info) {
      if (has(authorities, id)) {
        return true;
      }
      validation_protocol_.get()->reserve(info.peer.id, false);
      return false;
    });
    for (auto it = peer_to_auth_cache_.begin();
         it != peer_to_auth_cache_.end();) {
      if (has(authorities, it->second)) {
        ++it;
      } else {
        it = peer_to_auth_cache_.erase(it);
        validation_protocol_.get()->reserve(it->first, false);
      }
    }
    std::shuffle(authorities.begin(), authorities.end(), random_);

    // reorder queue to query unknown authorities first
    queue_.resize(0);
    queue_.reserve(authorities.size());
    // `queue_` is a stack, so push known first
    for (auto &known : {true, false}) {
      for (auto &id : authorities) {
        if (audi_store_->contains(id) == known) {
          queue_.emplace_back(id);
        }
      }
    }

    pop();
    return outcome::success();
  }

  std::optional<primitives::AuthorityDiscoveryId> QueryImpl::hashToAuth(
      BufferView key) const {
    if (auto r = Hash256::fromSpan(key)) {
      auto it = hash_to_auth_.find(r.value());
      if (it != hash_to_auth_.end()) {
        return it->second;
      }
    }
    return std::nullopt;
  }

  void QueryImpl::pop() {
    while (active_ < kMaxActiveRequests) {
      if (queue_.empty()) {
        return;
      }
      ++active_;
      auto authority = queue_.back();
      queue_.pop_back();

      scheduler_->schedule([wp{weak_from_this()},
                            hash = common::Buffer{crypto::sha256(authority)},
                            authority] {
        if (auto self = wp.lock()) {
          SL_DEBUG(
              self->log_, "start lookup({})", common::hex_lower(authority));
          std::ignore = self->kademlia_.get()->getValue(
              hash, [=](const outcome::result<std::vector<uint8_t>> &res) {
                if (auto self = wp.lock()) {
                  std::unique_lock lock{self->mutex_};
                  --self->active_;
                  self->pop();
                }
              });
        }
      });
    }
  }

  outcome::result<void> QueryImpl::add(
      const primitives::AuthorityDiscoveryId &authority,
      outcome::result<std::vector<uint8_t>> _res) {
    SL_TRACE(log_,
             "lookup : add addresses for authority {}, _res {}",
             common::hex_lower(authority),
             _res.has_value() ? "ok" : "error: " + _res.error().message());
    OUTCOME_TRY(signed_record_pb, _res);
    auto it = audi_store_->get(authority);
    if (it != std::nullopt and signed_record_pb == it->raw) {
      return outcome::success();
    }
    ::authority_discovery_v3::SignedAuthorityRecord signed_record;
    if (not signed_record.ParseFromArray(
            signed_record_pb.data(),
            // NOLINTNEXTLINE(cppcoreguidelines-narrowing-conversions)
            signed_record_pb.size())) {
      SL_ERROR(log_,
               "lookup: can't parse signed record from authority {}",
               authority);
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

    ::authority_discovery_v3::AuthorityRecord record;
    if (not record.ParseFromString(signed_record.record())) {
      SL_TRACE(log_, "lookup: can't parse record from authority {}", authority);
      return Error::DECODE_ERROR;
    }
    if (record.addresses().empty()) {
      SL_ERROR(log_, "lookup: no addresses from authority {}", authority);
      return Error::NO_ADDRESSES;
    }
    std::optional<Timestamp> time{};
    if (record.has_creation_time()) {
      OUTCOME_TRY(tmp,
                  scale::decode<TimestampScale>(
                      qtils::str2byte(record.creation_time().timestamp())));
      time = *tmp;
      if (it and it->time and time <= it->time->number) {
        SL_TRACE(log_, "lookup: outdated record for authority {}", authority);
        return outcome::success();
      }
    }
    scale::PeerInfoSerializable peer;
    peer.id = std::move(peer_id);
    auto peer_id_str = peer.id.toBase58();
    SL_TRACE(log_,
             "lookup: adding {} addresses for authority {}",
             record.addresses().size(),
             authority);
    for (auto &pb : record.addresses()) {
      OUTCOME_TRY(address, libp2p::multi::Multiaddress::create(str2byte(pb)));
      auto id = address.getPeerId();
      if (not id) {
        continue;
      }
      if (id != peer_id_str) {
        SL_ERROR(log_,
                 "lookup: inconsistent peer id {} != {}",
                 id.value(),
                 peer_id_str);
        return Error::INCONSISTENT_PEER_ID;
      }
      peer.addresses.emplace_back(std::move(address));
    }

    OUTCOME_TRY(auth_sig_ok,
                sr_crypto_provider_->verify(
                    auth_sig, str2byte(signed_record.record()), authority));
    if (not auth_sig_ok) {
      SL_ERROR(log_, "lookup: invalid authority signature");
      return Error::INVALID_SIGNATURE;
    }

    OUTCOME_TRY(peer_sig_ok,
                libp2p_crypto_provider_->verify(
                    str2byte(signed_record.record()),
                    str2byte(signed_record.peer_signature().signature()),
                    peer_key));
    if (not peer_sig_ok) {
      SL_ERROR(log_, "lookup: invalid peer signature");
      return Error::INVALID_SIGNATURE;
    }

    std::ignore = host_.getPeerRepository().getAddressRepository().addAddresses(
        peer.id, peer.addresses, libp2p::peer::ttl::kDay);

    peer_to_auth_cache_.insert_or_assign(peer.id, authority);
    audi_store_->store(
        authority,
        AuthorityPeerInfo{
            .raw = std::move(signed_record_pb),
            .time = time.has_value() ? std::make_optional(TimestampScale{*time})
                                     : std::nullopt,
            .peer = peer,
        });

    validation_protocol_.get()->reserve(peer.id, true);

    return outcome::success();
  }
}  // namespace kagome::authority_discovery
