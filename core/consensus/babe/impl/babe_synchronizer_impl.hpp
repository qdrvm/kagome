/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 *
 * Kagome is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * Kagome is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef KAGOME_BABE_SYNCHRONIZER_IMPL_HPP
#define KAGOME_BABE_SYNCHRONIZER_IMPL_HPP

#include "consensus/babe/babe_synchronizer.hpp"

#include "common/logger.hpp"
#include "network/types/sync_clients_set.hpp"

namespace kagome::consensus {

  /**
   * Implementation of babe synchronizer that requests blocks from provided
   * peers
   */
  class BabeSynchronizerImpl
      : public BabeSynchronizer,
        public std::enable_shared_from_this<BabeSynchronizerImpl> {
   public:
    ~BabeSynchronizerImpl() override = default;

    explicit BabeSynchronizerImpl(
        std::shared_ptr<network::SyncClientsSet> sync_clients);

    void request(const primitives::BlockId &from,
                 const primitives::BlockHash &to,
                 const BlocksHandler &block_list_handler) override;

   private:
    /**
     * Select next client to be polled
     * @param polled_clients clients that we already polled
     * @return next clint to be polled
     */
    std::shared_ptr<network::SyncProtocolClient> selectNextClient(
        std::unordered_set<std::shared_ptr<network::SyncProtocolClient>>
            &polled_clients) const;
    /**
     * Request blocks from provided peers
     * @param request block request message
     * @param polled_clients peers that were already requested
     * @param requested_blocks_handler handler of received blocks
     */
    void pollClients(
        network::BlocksRequest request,
        std::unordered_set<std::shared_ptr<network::SyncProtocolClient>>
            &&polled_clients,
        const BlocksHandler &requested_blocks_handler) const;

    std::shared_ptr<network::SyncClientsSet> sync_clients_;
    common::Logger logger_;
  };
}  // namespace kagome::consensus

#endif  // KAGOME_BABE_SYNCHRONIZER_IMPL_HPP
