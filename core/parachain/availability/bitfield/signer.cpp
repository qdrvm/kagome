/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "parachain/availability/bitfield/signer.hpp"

#include "log/logger.hpp"
#include "parachain/availability/availability_chunk_index.hpp"
#include "primitives/block_header.hpp"

namespace kagome::parachain {
  constexpr std::chrono::milliseconds kDelay{1500};

  BitfieldSigner::BitfieldSigner(
      std::shared_ptr<crypto::Hasher> hasher,
      std::shared_ptr<ValidatorSignerFactory> signer_factory,
      std::shared_ptr<libp2p::basic::Scheduler> scheduler,
      std::shared_ptr<runtime::ParachainHost> parachain_api,
      std::shared_ptr<AvailabilityStore> store,
      std::shared_ptr<Fetch> fetch,
      std::shared_ptr<BitfieldStore> bitfield_store,
      primitives::events::ChainSubscriptionEnginePtr chain_sub_engine)
      : hasher_{std::move(hasher)},
        signer_factory_{std::move(signer_factory)},
        scheduler_{std::move(scheduler)},
        parachain_api_{std::move(parachain_api)},
        store_{std::move(store)},
        fetch_{std::move(fetch)},
        bitfield_store_{std::move(bitfield_store)},
        chain_sub_{std::move(chain_sub_engine)} {}

  void BitfieldSigner::start() {
    chain_sub_.onHead(
        [weak{weak_from_this()}](const primitives::BlockHeader &header) {
          if (auto self = weak.lock()) {
            auto r = self->onBlock(header.hash());
            if (r.has_error()) {
              SL_DEBUG(self->logger_, "onBlock error {}", r.error());
            }
          }
        });
  }

  void BitfieldSigner::setBroadcastCallback(
      BitfieldSigner::BroadcastCallback &&callback) {
    BOOST_ASSERT_MSG(broadcast_ == nullptr, "Callback already stored.");
    broadcast_ = std::move(callback);
  }

  outcome::result<void> BitfieldSigner::sign(const ValidatorSigner &signer,
                                             const Candidates &candidates) {
    const BlockHash &relay_parent = signer.relayParent();
    scale::BitVector bitfield;
    bitfield.reserve(candidates.size());
    for (auto &candidate : candidates) {
      bitfield.push_back(
          candidate && store_->hasChunk(*candidate, signer.validatorIndex()));
    }

    OUTCOME_TRY(signed_bitfield, signer.sign(bitfield));
    bitfield_store_->putBitfield(relay_parent, signed_bitfield);

    BOOST_ASSERT_MSG(broadcast_ != nullptr,
                     "No broadcast callback registered.");
    broadcast_(relay_parent, signed_bitfield);

    return outcome::success();
  }

  outcome::result<void> BitfieldSigner::onBlock(const BlockHash &relay_parent) {
    OUTCOME_TRY(signer, signer_factory_->at(relay_parent));
    if (not signer.has_value()) {
      return outcome::success();
    }
    Candidates candidates;
    OUTCOME_TRY(cores, parachain_api_->availability_cores(relay_parent));
    OUTCOME_TRY(
        session,
        parachain_api_->session_info(relay_parent, signer->getSessionIndex()));
    OUTCOME_TRY(node_features, parachain_api_->node_features(relay_parent));
    candidates.reserve(cores.size());
    for (CoreIndex core_index = 0; core_index < cores.size(); ++core_index) {
      auto &core = cores[core_index];
      if (auto occupied = std::get_if<runtime::OccupiedCore>(&core)) {
        candidates.emplace_back(occupied->candidate_hash);

        size_t n_validators = 0;
        for (const auto &group : session->validator_groups) {
          n_validators += group.size();
        }

        SL_DEBUG(logger_,
                 "chunk mapping is enabled: {}",
                 availability_chunk_mapping_is_enabled(node_features) ? "YES"
                                                                      : "NO");
        OUTCOME_TRY(chunk_index,
                    availability_chunk_index(node_features,
                                             n_validators,
                                             core_index,
                                             signer->validatorIndex()));
        fetch_->fetch(chunk_index, *occupied, *session);
      } else {
        candidates.emplace_back(std::nullopt);
      }
    }

    scheduler_->schedule(
        [weak{weak_from_this()},
         signer{std::move(*signer)},
         candidates{std::move(candidates)}]() mutable {
          if (auto self = weak.lock()) {
            auto r = self->sign(signer, candidates);
            if (r.has_error()) {
              SL_WARN(self->logger_, "sign error {}", r.error());
            }
          }
        },
        kDelay);
    return outcome::success();
  }
}  // namespace kagome::parachain
