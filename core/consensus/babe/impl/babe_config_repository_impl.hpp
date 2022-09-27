/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CONSENSUS_BABE_BABECONFIGREPOSITORYIMPL
#define KAGOME_CONSENSUS_BABE_BABECONFIGREPOSITORYIMPL

#include "consensus/babe/babe_config_repository.hpp"
#include "primitives/event_types.hpp"

namespace kagome::application {
  class AppStateManager;
}
namespace kagome::crypto {
  class Hasher;
}
namespace kagome::runtime {
  class BabeApi;
}

namespace kagome::consensus::babe {

  class BabeConfigRepositoryImpl final
      : public BabeConfigRepository,
        public std::enable_shared_from_this<BabeConfigRepositoryImpl> {
   public:
    BabeConfigRepositoryImpl(
        const std::shared_ptr<application::AppStateManager> &app_state_manager,
        std::shared_ptr<runtime::BabeApi> babe_api,
        std::shared_ptr<crypto::Hasher> hasher,
        primitives::events::ChainSubscriptionEnginePtr,
        primitives::BlockHash genesis_block_hash);

    const primitives::BabeConfiguration &config() override;

    bool prepare();

   private:
    std::shared_ptr<runtime::BabeApi> babe_api_;
    std::shared_ptr<crypto::Hasher> hasher_;
    std::shared_ptr<primitives::events::ChainEventSubscriber> chain_sub_;
    primitives::BlockHash block_hash_;

    mutable primitives::BabeConfiguration babe_configuration_;
    bool valid_;
  };

}  // namespace kagome::consensus::babe

#endif  // KAGOME_CONSENSUS_BABE_BABECONFIGREPOSITORYIMPL
