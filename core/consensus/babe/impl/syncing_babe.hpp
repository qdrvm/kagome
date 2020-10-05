/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CONSENSUS_BABESYNCING
#define KAGOME_CONSENSUS_BABESYNCING

#include "consensus/babe/babe.hpp"
#include "consensus/babe/impl/block_executor.hpp"

namespace kagome::consensus {

  class SyncingBabe : public Babe {
   public:
    explicit SyncingBabe(
        std::shared_ptr<consensus::BlockExecutor> block_executor);
    ~SyncingBabe() override = default;

    bool start() {
      return true;
    }

    void setExecutionStrategy(ExecutionStrategy strategy) override {}

    void runEpoch(Epoch epoch,
                  BabeTimePoint starting_slot_finish_time) override{};

    State getCurrentState() const override {
      return State::WAIT_BLOCK;
    };

    void onBlockAnnounce(const network::BlockAnnounce &announce) override;

    void doOnSynchronized(std::function<void()> handler) override{};

   private:
    std::shared_ptr<consensus::BlockExecutor> block_executor_;
  };
}  // namespace kagome::consensus

#endif  // KAGOME_CONSENSUS_BABESYNCING
