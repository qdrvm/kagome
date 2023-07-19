/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "parachain/pvf/precheck.hpp"

#include "offchain/offchain_worker_factory.hpp"
#include "offchain/offchain_worker_pool.hpp"
#include "runtime/common/uncompress_code_if_needed.hpp"
#include "runtime/module.hpp"
#include "runtime/module_factory.hpp"

namespace kagome::parachain {
  PvfPrecheck::PvfPrecheck(
      std::shared_ptr<crypto::Hasher> hasher,
      std::shared_ptr<ValidatorSignerFactory> signer_factory,
      std::shared_ptr<runtime::ParachainHost> parachain_api,
      std::shared_ptr<runtime::ModuleFactory> module_factory,
      std::shared_ptr<runtime::Executor> executor,
      std::shared_ptr<offchain::OffchainWorkerFactory> offchain_worker_factory,
      std::shared_ptr<offchain::OffchainWorkerPool> offchain_worker_pool)
      : hasher_{std::move(hasher)},
        signer_factory_{std::move(signer_factory)},
        parachain_api_{std::move(parachain_api)},
        module_factory_{std::move(module_factory)},
        executor_{std::move(executor)},
        offchain_worker_factory_{std::move(offchain_worker_factory)},
        offchain_worker_pool_{std::move(offchain_worker_pool)} {}

  void PvfPrecheck::start(
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
            self->thread_.io_context()->post(
                [weak,
                 header{boost::get<primitives::events::HeadsEventParams>(event)
                            .get()}] {
                  if (auto self = weak.lock()) {
                    auto block_hash = self->hasher_->blake2b_256(
                        scale::encode(header).value());
                    auto r = self->onBlock(block_hash, header);
                    if (r.has_error()) {
                      SL_DEBUG(self->logger_, "onBlock error {}", r.error());
                    }
                  }
                });
          }
        });
  }

  outcome::result<void> PvfPrecheck::onBlock(
      const BlockHash &block_hash, const primitives::BlockHeader &header) {
    OUTCOME_TRY(signer, signer_factory_->at(block_hash));
    if (not signer.has_value()) {
      return outcome::success();
    }
    OUTCOME_TRY(need, parachain_api_->pvfs_require_precheck(block_hash));
    for (auto &code_hash : need) {
      if (not seen_.emplace(code_hash).second) {
        continue;
      }
      auto code_zstd_res =
          parachain_api_->validation_code_by_hash(block_hash, code_hash);
      if (not code_zstd_res or not code_zstd_res.value()) {
        seen_.erase(code_hash);
        continue;
      }
      auto &code_zstd = *code_zstd_res.value();
      ParachainRuntime code;
      auto res = [&]() -> outcome::result<void> {
        OUTCOME_TRY(runtime::uncompressCodeIfNeeded(code_zstd, code));
        OUTCOME_TRY(module_factory_->make(code));
        return outcome::success();
      }();
      PvfCheckStatement statement{
          res.has_value(),
          code_hash,
          signer->getSessionIndex(),
          signer->validatorIndex(),
      };
      OUTCOME_TRY(signature, signer->signRaw(statement.signable()));
      offchain_worker_pool_->addWorker(
          offchain_worker_factory_->make(executor_, header));
      auto remove =
          gsl::finally([&] { offchain_worker_pool_->removeWorker(); });
      OUTCOME_TRY(parachain_api_->submit_pvf_check_statement(
          block_hash, statement, signature));
    }
    return outcome::success();
  }
}  // namespace kagome::parachain
