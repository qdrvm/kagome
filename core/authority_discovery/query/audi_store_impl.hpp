/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "authority_discovery/query/audi_store.hpp"
#include "log/logger.hpp"
#include "primitives/authority_discovery_id.hpp"
#include "storage/spaced_storage.hpp"

namespace kagome::authority_discovery {

  class AudiStoreImpl : public AudiStore {
   public:
    explicit AudiStoreImpl(std::shared_ptr<storage::SpacedStorage> storage);

    ~AudiStoreImpl() override = default;

    void store(const primitives::AuthorityDiscoveryId &authority,
               const AuthorityPeerInfo &data) override;

    std::optional<AuthorityPeerInfo> get(
        const primitives::AuthorityDiscoveryId &authority) const override;

    outcome::result<void> remove(
        const primitives::AuthorityDiscoveryId &authority) override;

    bool contains(
        const primitives::AuthorityDiscoveryId &authority) const override;

    void forEach(
        std::function<void(const primitives::AuthorityDiscoveryId &,
                           const AuthorityPeerInfo &)> f) const override;

    void retainIf(std::function<bool(const primitives::AuthorityDiscoveryId &,
                                     const AuthorityPeerInfo &)> f) override;

   private:
    std::shared_ptr<storage::BufferStorage> space_;
    log::Logger log_;
  };

}  // namespace kagome::authority_discovery
