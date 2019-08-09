/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_SYNCHRONIZER_IMPL_HPP
#define KAGOME_SYNCHRONIZER_IMPL_HPP

#include <memory>

#include "blockchain/block_tree.hpp"
#include "common/logger.hpp"
#include "consensus/synchronizer/synchronizer.hpp"

namespace kagome::consensus {
  class SynchronizerImpl : public Synchronizer {
   public:
    explicit SynchronizerImpl(
        std::shared_ptr<blockchain::BlockTree> block_tree,
        common::Logger log = common::createLogger("Synchronizer"));

    void announce(const primitives::Block &block) override;

    void requestBlocks(const libp2p::peer::PeerInfo &peer,
                       RequestCallback cb) override;

    void requestBlocks(const primitives::BlockHash &hash,
                       primitives::BlockNumber number,
                       RequestCallback cb) override;

   private:
    std::shared_ptr<blockchain::BlockTree> block_tree_;
    common::Logger log_;
  };
}  // namespace kagome::consensus

#endif  // KAGOME_SYNCHRONIZER_IMPL_HPP
