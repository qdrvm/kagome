/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CONSENSUS_BABE_BABECONFIGREPOSITORYIMPL
#define KAGOME_CONSENSUS_BABE_BABECONFIGREPOSITORYIMPL

#include "consensus/babe/babe_config_repository.hpp"
#include "consensus/babe/babe_digest_observer.hpp"
#include "consensus/babe/babe_util.hpp"

#include "consensus/babe/impl/babe_config_node.hpp"
#include "log/logger.hpp"
#include "primitives/block_data.hpp"
#include "primitives/event_types.hpp"
#include "storage/buffer_map_types.hpp"

namespace kagome::application {
  class AppStateManager;
}
namespace kagome::blockchain {
  class BlockTree;
  class BlockHeaderRepository;
}  // namespace kagome::blockchain
namespace kagome::crypto {
  class Hasher;
}
namespace kagome::runtime {
  class BabeApi;
}

namespace kagome::consensus::babe {

  class BabeConfigRepositoryImpl final
      : public BabeConfigRepository,
        public BabeDigestObserver,
        public BabeUtil,
        public std::enable_shared_from_this<BabeConfigRepositoryImpl> {
    static const primitives::BlockNumber kSavepointBlockInterval = 100000;

   public:
    BabeConfigRepositoryImpl(
        const std::shared_ptr<application::AppStateManager> &app_state_manager,
        std::shared_ptr<storage::BufferStorage> persistent_storage,
        std::shared_ptr<blockchain::BlockTree> block_tree,
        std::shared_ptr<blockchain::BlockHeaderRepository> header_repo,
        std::shared_ptr<runtime::BabeApi> babe_api,
        std::shared_ptr<crypto::Hasher> hasher,
        primitives::events::ChainSubscriptionEnginePtr chain_events_engine,
        const BabeClock &clock);

    bool prepare();

    // BabeDigestObserver

    outcome::result<void> onDigest(const primitives::BlockContext &context,
                                   const BabeBlockHeader &digest) override;

    outcome::result<void> onDigest(
        const primitives::BlockContext &context,
        const primitives::BabeDigest &digest) override;

    void cancel(const primitives::BlockInfo &block) override;

    // BabeConfigRepository

    BabeDuration slotDuration() const override;

    EpochLength epochLength() const override;

    std::shared_ptr<const primitives::BabeConfiguration> config(
        const primitives::BlockContext &context,
        EpochNumber epoch_number) override;

    // BabeUtil

    BabeSlotNumber syncEpoch(
        std::function<std::tuple<BabeSlotNumber, bool>()> &&f) override;

    BabeSlotNumber getCurrentSlot() const override;

    BabeTimePoint slotStartTime(BabeSlotNumber slot) const override;
    BabeDuration remainToStartOfSlot(BabeSlotNumber slot) const override;
    BabeTimePoint slotFinishTime(BabeSlotNumber slot) const override;
    BabeDuration remainToFinishOfSlot(BabeSlotNumber slot) const override;

    EpochNumber slotToEpoch(BabeSlotNumber slot) const override;
    BabeSlotNumber slotInEpoch(BabeSlotNumber slot) const override;

   private:
    outcome::result<void> load();
    outcome::result<void> save();

    void prune(const primitives::BlockInfo &block);

    outcome::result<void> onNextEpochData(
        const primitives::BlockContext &context,
        const primitives::NextEpochData &msg);

    outcome::result<void> onNextConfigData(
        const primitives::BlockContext &context,
        const primitives::NextConfigDataV1 &msg);

    /**
     * @brief Find node according to the block
     * @param block for which to find the schedule node
     * @return oldest node according to the block
     */
    std::shared_ptr<BabeConfigNode> getNode(
        const primitives::BlockContext &context) const;

    /**
     * @brief Check if one block is direct ancestor of second one
     * @param ancestor - hash of block, which is at the top of the chain
     * @param descendant - hash of block, which is the bottom of the chain
     * @return true if \param ancestor is direct ancestor of \param descendant
     */
    bool directChainExists(const primitives::BlockInfo &ancestor,
                           const primitives::BlockInfo &descendant) const;

    BabeSlotNumber getFirstBlockSlotNumber();

    std::shared_ptr<storage::BufferStorage> persistent_storage_;
    std::shared_ptr<blockchain::BlockTree> block_tree_;
    std::shared_ptr<blockchain::BlockHeaderRepository> header_repo_;
    std::shared_ptr<runtime::BabeApi> babe_api_;
    std::shared_ptr<crypto::Hasher> hasher_;
    std::shared_ptr<primitives::events::ChainEventSubscriber> chain_sub_;

    const BabeDuration slot_duration_{};
    const EpochLength epoch_length_{};

    std::shared_ptr<BabeConfigNode> root_;
    primitives::BlockNumber last_saved_state_block_ = 0;

    const BabeClock &clock_;
    std::optional<BabeSlotNumber> first_block_slot_number_;
    bool is_first_block_finalized_ = false;

    log::Logger logger_;
  };

}  // namespace kagome::consensus::babe

#endif  // KAGOME_CONSENSUS_BABE_BABECONFIGREPOSITORYIMPL
