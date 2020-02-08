/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_BABE_OBSERVER_IMPL_HPP
#define KAGOME_BABE_OBSERVER_IMPL_HPP

#include <memory>

#include "blockchain/block_tree.hpp"
#include "common/logger.hpp"
#include "consensus/babe/epoch_storage.hpp"
#include "consensus/validation/block_validator.hpp"
#include "network/babe_observer.hpp"
#include "network/types/sync_clients_set.hpp"

namespace kagome::consensus {
  class BabeObserverImpl
      : public network::BabeObserver,
        public std::enable_shared_from_this<BabeObserverImpl> {
   public:
    BabeObserverImpl(std::shared_ptr<BlockValidator> validator,
                     std::shared_ptr<network::SyncClientsSet> sync_clients,
                     std::shared_ptr<blockchain::BlockTree> tree,
                     std::shared_ptr<EpochStorage> epoch_storage);

    ~BabeObserverImpl() override = default;

    void onBlockAnnounce(const network::BlockAnnounce &announce) const override;

   private:
    void pollClients(
        primitives::Block block_to_insert,
        network::BlocksRequest request,
        std::shared_ptr<
            std::unordered_set<std::shared_ptr<network::SyncProtocolClient>>>
            polled_clients) const;

    std::shared_ptr<BlockValidator> validator_;
    std::shared_ptr<network::SyncClientsSet> sync_clients_;
    std::shared_ptr<blockchain::BlockTree> tree_;
    std::shared_ptr<EpochStorage> epoch_storage_;
    common::Logger logger_;
  };
}  // namespace kagome::consensus

#endif  // KAGOME_BABE_OBSERVER_IMPL_HPP
