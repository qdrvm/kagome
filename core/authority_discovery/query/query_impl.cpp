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
#include "utils/retain_if.hpp"
#include "utils/weak_macro.hpp"

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
      std::shared_ptr<rust_kad::Kad> rust_kad,
      std::shared_ptr<runtime::AuthorityDiscoveryApi> authority_discovery_api,
      std::shared_ptr<crypto::KeyStore> key_store,
      std::shared_ptr<crypto::Sr25519Provider> sr_crypto_provider,
      std::shared_ptr<libp2p::crypto::CryptoProvider> libp2p_crypto_provider,
      std::shared_ptr<libp2p::crypto::marshaller::KeyMarshaller> key_marshaller,
      libp2p::Host &host,
      LazySPtr<libp2p::protocol::kademlia::Kademlia> kademlia,
      std::shared_ptr<libp2p::basic::Scheduler> scheduler)
      : block_tree_{std::move(block_tree)},
        rust_kad_{std::move(rust_kad)},
        authority_discovery_api_{std::move(authority_discovery_api)},
        key_store_{std::move(key_store)},
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
  }

  /**
   * @brief Starts the query process.
   *
   * This method initiates the periodic update process by starting the interval
   * timer. The `update` method is called at each interval, and any errors
   * encountered during the update are logged.
   *
   * @return true if the process starts successfully.
   */
  bool QueryImpl::start() {
    interval_.start([this] {
      auto r = update();
      if (not r) {
        SL_WARN(log_, "update: {}", r.error());
      }
    });
    return true;
  }

  /**
   * @brief Retrieves the peer information for a given authority.
   *
   * This method looks up the peer information associated with the specified
   * authority in the `auth_to_peer_cache_`. If the authority is found, it
   * returns the corresponding peer information. Otherwise, it returns
   * `std::nullopt`.
   *
   * @param authority The authority discovery ID for which to retrieve the peer
   * information.
   * @return An optional containing the peer information if found, or
   * `std::nullopt` if not found.
   */
  std::optional<libp2p::peer::PeerInfo> QueryImpl::get(
      const primitives::AuthorityDiscoveryId &authority) const {
    std::unique_lock lock{mutex_};
    auto it = auth_to_peer_cache_.find(authority);
    if (it != auth_to_peer_cache_.end()) {
      return it->second.peer;
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
    auto it = auth_to_peer_cache_.find(*id);
    if (it != auth_to_peer_cache_.end()) {
      auto it_value = std::ranges::find(values, it->second.raw);
      if (it_value != values.end()) {
        return it_value - values.begin();
      }
    }
    return Error::KADEMLIA_OUTDATED_VALUE;
  }

  /**
   * @brief Updates the authority discovery information.
   *
   * This method retrieves the latest authority discovery information from the
   * blockchain and updates the internal caches. It ensures that the caches are
   * consistent with the current state of the blockchain and removes any
   * outdated entries. The method also shuffles the list of authorities and
   * prepares the queue for processing.
   *
   * @return outcome::result<void> indicating the success or failure of the
   * update operation.
   */
  outcome::result<void> QueryImpl::update() {
    // Acquire a unique lock to ensure thread safety
    std::unique_lock lock{mutex_};

    // Retrieve the list of authorities from the blockchain
    OUTCOME_TRY(
        authorities,
        authority_discovery_api_->authorities(block_tree_->bestBlock().hash));

    // Update the hash-to-authority mapping
    for (auto &id : authorities) {
      if (not hash_to_auth_.contains(id)) {
        hash_to_auth_.emplace(crypto::sha256(id), id);
      }
    }

    // Retrieve the local keys from the key store
    OUTCOME_TRY(local_keys,
                key_store_->sr25519().getPublicKeys(
                    crypto::KeyTypes::AUTHORITY_DISCOVERY));

    // Helper function to check if a key is in a list of keys
    auto has = [](const std::vector<primitives::AuthorityDiscoveryId> &keys,
                  const primitives::AuthorityDiscoveryId &key) {
      return std::ranges::find(keys, key) != keys.end();
    };

    // Remove local keys from the list of authorities
    retain_if(authorities, [&](const primitives::AuthorityDiscoveryId &id) {
      return not has(local_keys, id);
    });

    // Remove outdated entries from the authority-to-peer cache
    for (auto it = auth_to_peer_cache_.begin();
         it != auth_to_peer_cache_.end();) {
      if (has(authorities, it->first)) {
        ++it;
      } else {
        it = auth_to_peer_cache_.erase(it);
      }
    }

    // Remove outdated entries from the peer-to-authority cache
    for (auto it = peer_to_auth_cache_.begin();
         it != peer_to_auth_cache_.end();) {
      if (has(authorities, it->second)) {
        ++it;
      } else {
        it = peer_to_auth_cache_.erase(it);
      }
    }

    // Shuffle the list of authorities for random processing
    std::shuffle(authorities.begin(), authorities.end(), random_);

    // Update the queue with the shuffled list of authorities
    queue_ = std::move(authorities);

    // Process the queue to handle the authorities
    pop();

    // Return success
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
                            authority,
                            this] {
        if (auto self = wp.lock()) {
          SL_INFO(self->log_, "start lookup({})", common::hex_lower(authority));
          return self->rust_kad_->lookup(
              hash, [authority, WEAK_SELF](std::vector<Buffer> values) {
                WEAK_LOCK(self);
                if (values.empty()) {
                  SL_WARN(self->log_,
                          "lookup({}): no values",
                          common::hex_lower(authority));
                } else {
                  SL_INFO(self->log_,
                          "lookup({}) finished: {} values",
                          common::hex_lower(authority),
                          values.size());
                }
                std::unique_lock lock{self->mutex_};
                --self->active_;
                self->pop();
                for (auto &value : values) {
                  auto r = self->add(authority, value);
                  if (not r) {
                    SL_WARN(self->log_,
                            "add({}): {}",
                            common::hex_lower(authority),
                            r.error());
                  }
                }
              });
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
    OUTCOME_TRY(signed_record_pb, _res);
    auto it = auth_to_peer_cache_.find(authority);
    if (it != auth_to_peer_cache_.end()
        and signed_record_pb == it->second.raw) {
      return outcome::success();
    }
    ::authority_discovery_v3::SignedAuthorityRecord signed_record;
    if (not signed_record.ParseFromArray(
            signed_record_pb.data(),
            // NOLINTNEXTLINE(cppcoreguidelines-narrowing-conversions)
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

    ::authority_discovery_v3::AuthorityRecord record;
    if (not record.ParseFromString(signed_record.record())) {
      SL_ERROR(log_, "Failed to parse authority record {}", authority);
      return Error::DECODE_ERROR;
    }
    if (record.addresses().empty()) {
      SL_ERROR(log_, "No addresses in authority record {}", authority);
      return Error::NO_ADDRESSES;
    }
    std::optional<Timestamp> time{};
    if (record.has_creation_time()) {
      OUTCOME_TRY(tmp,
                  scale::decode<TimestampScale>(
                      qtils::str2byte(record.creation_time().timestamp())));
      time = *tmp;
      if (it != auth_to_peer_cache_.end() and time <= it->second.time) {
        return outcome::success();
      }
    }
    libp2p::peer::PeerInfo peer{.id = std::move(peer_id)};
    auto peer_id_str = peer.id.toBase58();
    for (auto &pb : record.addresses()) {
      OUTCOME_TRY(address, libp2p::multi::Multiaddress::create(str2byte(pb)));
      auto id = address.getPeerId();
      if (not id) {
        continue;
      }
      if (id != peer_id_str) {
        SL_ERROR(log_,
                 "Inconsistent peer id {} in address {}",
                 id.value(),
                 address.getStringAddress());
        return Error::INCONSISTENT_PEER_ID;
      }
      peer.addresses.emplace_back(std::move(address));
      SL_INFO(log_,
              "Added address {} to peer {} authority {}",
              peer.addresses.back().getStringAddress(),
              peer_id_str,
              common::hex_lower(authority));
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

    std::ignore = host_.getPeerRepository().getAddressRepository().addAddresses(
        peer.id, peer.addresses, libp2p::peer::ttl::kDay);

    peer_to_auth_cache_.insert_or_assign(peer.id, authority);
    auth_to_peer_cache_.insert_or_assign(authority,
                                         Authority{
                                             .raw = std::move(signed_record_pb),
                                             .time = time,
                                             .peer = std::move(peer),
                                         });

    return outcome::success();
  }
}  // namespace kagome::authority_discovery
