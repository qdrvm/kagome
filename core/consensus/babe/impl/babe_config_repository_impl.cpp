/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "babe_config_repository_impl.hpp"

#include "application/app_state_manager.hpp"
#include "crypto/hasher.hpp"
#include "primitives/block_header.hpp"
#include "runtime/runtime_api/babe_api.hpp"
#include "scale.hpp"

namespace kagome::consensus::babe {

  BabeConfigRepositoryImpl::BabeConfigRepositoryImpl(
      const std::shared_ptr<application::AppStateManager> &app_state_manager,
      std::shared_ptr<runtime::BabeApi> babe_api,
      std::shared_ptr<crypto::Hasher> hasher,

      primitives::events::ChainSubscriptionEnginePtr chain_events_engine,
      primitives::BlockHash genesis_block_hash)
      : babe_api_(std::move(babe_api)),
        hasher_(std::move(hasher)),
        chain_sub_([&] {
          BOOST_ASSERT(chain_events_engine != nullptr);
          return std::make_shared<primitives::events::ChainEventSubscriber>(
              chain_events_engine);
        }()),
        block_hash_(std::move(genesis_block_hash)),
        valid_(false) {
    BOOST_ASSERT(babe_api_ != nullptr);
    BOOST_ASSERT(hasher_ != nullptr);

    BOOST_ASSERT(app_state_manager != nullptr);
    app_state_manager->atPrepare([this] { return prepare(); });
  }

  bool BabeConfigRepositoryImpl::prepare() {
    chain_sub_->subscribe(chain_sub_->generateSubscriptionSetId(),
                          primitives::events::ChainEventType::kFinalizedHeads);
    chain_sub_->setCallback([wp = weak_from_this()](
                                subscription::SubscriptionSetId,
                                auto &&,
                                primitives::events::ChainEventType type,
                                const primitives::events::ChainEventParams
                                    &event) {
      if (type != primitives::events::ChainEventType::kFinalizedHeads) {
        return;
      }
      if (auto self = wp.lock()) {
        auto hash = self->hasher_->blake2b_256(
            scale::encode(
                boost::get<primitives::events::HeadsEventParams>(event).get())
                .value());
        self->block_hash_ = hash;
        self->valid_ = false;
      }
    });

    return true;
  }

  const primitives::BabeConfiguration &BabeConfigRepositoryImpl::config() {
    if (not valid_) {
      auto babe_config_res = babe_api_->configuration(block_hash_);
      if (babe_config_res.has_value()) {
        babe_configuration_ = std::move(babe_config_res.value());
        valid_ = true;
      }
    }
    return babe_configuration_;
  }

}  // namespace kagome::consensus::babe
