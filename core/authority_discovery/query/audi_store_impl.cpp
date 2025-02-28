/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "authority_discovery/query/audi_store_impl.hpp"

#include <scale/kagome_scale.hpp>

namespace kagome::authority_discovery {

  AudiStoreImpl::AudiStoreImpl(std::shared_ptr<storage::SpacedStorage> storage)
      : space_{storage->getSpace(storage::Space::kAudiPeers)},
        logger_{log::createLogger("AudiStore", "authority_discovery")} {
    BOOST_ASSERT(space_ != nullptr);
  }

  void AudiStoreImpl::store(const primitives::AuthorityDiscoveryId &authority,
                            const AuthorityPeerInfo &data) {
    auto encoded = scale::encode(data);

    if (not encoded) {
      SL_ERROR(log_, "Failed to encode PeerInfo");
      return;
    }

    if (auto res = space_->put(authority, common::Buffer(encoded.value()));
        not res) {
      SL_ERROR(
          log_, "Failed to put authority {} error {}", authority, res.error());
    }
  }

  std::optional<AuthorityPeerInfo> AudiStoreImpl::get(
      const primitives::AuthorityDiscoveryId &authority) const {
    auto res = space_->tryGet(authority);

    // check if there was any critical error during reading
    if (not res) {
      SL_CRITICAL(log_,
                  "Failed to get authority {} due to database error {}",
                  authority,
                  res.error());
      return std::nullopt;
    }

    // check if the authority was not found
    if (not res.value()) {
      return std::nullopt;
    }

    auto decoded = scale::decode<AuthorityPeerInfo>(res.value().value());

    if (not decoded) {
      SL_ERROR(log_, "Failed to decode PeerInfo");
      return std::nullopt;
    }

    return decoded.value();
  }

  outcome::result<void> AudiStoreImpl::remove(
      const primitives::AuthorityDiscoveryId &authority) {
    return space_->remove(authority);
  }

  bool AudiStoreImpl::contains(
      const primitives::AuthorityDiscoveryId &authority) const {
    auto res = space_->tryGet(authority);
    return res.has_value() and res.value().has_value();
  }

  void AudiStoreImpl::forEach(
      std::function<void(const primitives::AuthorityDiscoveryId &,
                         const AuthorityPeerInfo &)> f) const {
    auto cursor = space_->cursor();
    std::ignore = cursor->seekFirst();
    while (cursor->isValid()) {
      auto key = cursor->key();
      auto value = cursor->value();
      if (not key or not value
          or key.value().size() != primitives::AuthorityDiscoveryId::size()) {
        std::ignore = cursor->next();
        continue;
      }
      primitives::AuthorityDiscoveryId authority =
          primitives::AuthorityDiscoveryId::fromSpan(key.value().toVector())
              .value();
      if (auto decoded = scale::decode<AuthorityPeerInfo>(value.value())) {
        f(authority, decoded.value());
      } else {
        SL_ERROR(log_, "Failed to decode PeerInfo");
      }
      std::ignore = cursor->next();
    }
  }

  void AudiStoreImpl::retainIf(
      std::function<bool(const primitives::AuthorityDiscoveryId &,
                         const AuthorityPeerInfo &)> f) {
    std::deque<primitives::AuthorityDiscoveryId> to_remove;
    forEach([&](const auto &authority, const auto &peer_info) {
      if (not f(authority, peer_info)) {
        to_remove.push_back(authority);
      }
    });
    for (const auto &authority : to_remove) {
      auto res = remove(authority);
      if (not res) {
        SL_ERROR(log_,
                 "Failed to remove authority {} due to db error {}",
                 authority,
                 res.error());
      }
    }
  }

}  // namespace kagome::authority_discovery
