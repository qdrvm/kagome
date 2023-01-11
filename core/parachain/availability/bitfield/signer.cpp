/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "parachain/availability/bitfield/signer.hpp"

#include "log/logger.hpp"
#include "primitives/block_header.hpp"

namespace kagome::parachain {
  constexpr std::chrono::milliseconds kDelay{1500};

  namespace {
    inline auto log() {
      return log::createLogger("BitfieldSigner");
    }
  }  // namespace

  BitfieldSigner::BitfieldSigner(
      std::shared_ptr<crypto::Hasher> hasher,
      std::shared_ptr<ValidatorSignerFactory> signer_factory,
      std::shared_ptr<libp2p::basic::Scheduler> scheduler,
      std::shared_ptr<runtime::ParachainHost> parachain_api,
      std::shared_ptr<AvailabilityStore> store,
      std::shared_ptr<BitfieldStore> bitfield_store)
      : hasher_{std::move(hasher)},
        signer_factory_{std::move(signer_factory)},
        scheduler_{std::move(scheduler)},
        parachain_api_{std::move(parachain_api)},
        store_{std::move(store)},
        bitfield_store_{std::move(bitfield_store)} {}

  void BitfieldSigner::start(
      std::shared_ptr<primitives::events::ChainSubscriptionEngine>
          chain_sub_engine) {
    chain_sub_ = std::make_shared<primitives::events::ChainEventSubscriber>(
        chain_sub_engine);
    chain_sub_->subscribe(chain_sub_->generateSubscriptionSetId(),
                          primitives::events::ChainEventType::kNewHeads);
    chain_sub_->setCallback(
        [weak = weak_from_this()](
            subscription::SubscriptionSetId,
            auto &&,
            primitives::events::ChainEventType,
            const primitives::events::ChainEventParams &event) {
          if (auto self = weak.lock()) {
            auto r = self->onBlock(self->hasher_->blake2b_256(
                scale::encode(
                    boost::get<primitives::events::HeadsEventParams>(event))
                    .value()));
            if (r.has_error()) {
              SL_WARN(log(), "onBlock error {}", r.error());
            }
          }
        });
  }

  outcome::result<void> BitfieldSigner::sign(const ValidatorSigner &signer) {
    auto &relay_parent = signer.relayParent();
    scale::BitVec bitfield;
    OUTCOME_TRY(cores, parachain_api_->availability_cores(relay_parent));
    bitfield.bits.reserve(cores.size());
    for (auto &core : cores) {
      auto occupied = boost::get<runtime::OccupiedCore>(&core);
      bitfield.bits.push_back(occupied != nullptr
                              && store_->hasChunk(occupied->candidate_hash,
                                                  signer.validatorIndex()));
    }

    OUTCOME_TRY(signed_bitfield, signer.sign(bitfield));
    bitfield_store_->putBitfield(relay_parent, signed_bitfield);
    // TODO(turuslan): broadcast
    return outcome::success();
  }

  outcome::result<void> BitfieldSigner::onBlock(const BlockHash &relay_parent) {
    OUTCOME_TRY(signer, signer_factory_->at(relay_parent));
    if (not signer.has_value()) {
      return outcome::success();
    }
    scheduler_->schedule(
        [weak = weak_from_this(), signer{std::move(*signer)}]() {
          if (auto self = weak.lock()) {
            auto r = self->sign(signer);
            if (r.has_error()) {
              SL_WARN(log(), "sign error {}", r.error());
            }
          }
        },
        kDelay);
    return outcome::success();
  }
}  // namespace kagome::parachain
