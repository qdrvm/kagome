/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "parachain/pvf/precheck.hpp"

#include <libp2p/common/final_action.hpp>

#include "blockchain/block_tree.hpp"
#include "metrics/histogram_timer.hpp"
#include "offchain/offchain_worker_factory.hpp"
#include "offchain/offchain_worker_pool.hpp"
#include "parachain/pvf/pvf_thread_pool.hpp"
#include "runtime/common/uncompress_code_if_needed.hpp"
#include "runtime/module.hpp"
#include "runtime/module_factory.hpp"

namespace kagome::parachain {
  constexpr size_t kSessions = 3;

  metrics::HistogramTimer metric_pvf_preparation_time{
      "kagome_pvf_preparation_time",
      "Time spent in preparing PVF artifacts in seconds",
      {
          0.1,
          0.5,
          1.0,
          2.0,
          3.0,
          10.0,
          20.0,
          30.0,
          60.0,
          120.0,
          240.0,
          360.0,
          480.0,
      },
  };

  PvfPrecheck::PvfPrecheck(
      std::shared_ptr<crypto::Hasher> hasher,
      std::shared_ptr<blockchain::BlockTree> block_tree,
      std::shared_ptr<ValidatorSignerFactory> signer_factory,
      std::shared_ptr<runtime::ParachainHost> parachain_api,
      std::shared_ptr<runtime::ModuleFactory> module_factory,
      std::shared_ptr<runtime::Executor> executor,
      PvfThreadPool &pvf_thread_pool,
      std::shared_ptr<offchain::OffchainWorkerFactory> offchain_worker_factory,
      std::shared_ptr<offchain::OffchainWorkerPool> offchain_worker_pool)
      : hasher_{std::move(hasher)},
        block_tree_{std::move(block_tree)},
        signer_factory_{std::move(signer_factory)},
        parachain_api_{std::move(parachain_api)},
        module_factory_{std::move(module_factory)},
        executor_{std::move(executor)},
        offchain_worker_factory_{std::move(offchain_worker_factory)},
        offchain_worker_pool_{std::move(offchain_worker_pool)},
        pvf_thread_handler_{pvf_thread_pool.handlerManual()} {
    BOOST_ASSERT(hasher_ != nullptr);
    BOOST_ASSERT(block_tree_ != nullptr);
    BOOST_ASSERT(signer_factory_ != nullptr);
    BOOST_ASSERT(parachain_api_ != nullptr);
    BOOST_ASSERT(module_factory_ != nullptr);
    BOOST_ASSERT(executor_ != nullptr);
    BOOST_ASSERT(offchain_worker_factory_ != nullptr);
    BOOST_ASSERT(offchain_worker_pool_ != nullptr);
  }

  void PvfPrecheck::start(
      std::shared_ptr<primitives::events::ChainSubscriptionEngine>
          chain_sub_engine) {
    pvf_thread_handler_->start();

    chain_sub_ = std::make_shared<primitives::events::ChainEventSubscriber>(
        chain_sub_engine);
    chain_sub_->subscribe(chain_sub_->generateSubscriptionSetId(),
                          primitives::events::ChainEventType::kNewHeads);
    chain_sub_->setCallback(
        [weak{weak_from_this()}](
            subscription::SubscriptionSetId,
            auto &&,
            primitives::events::ChainEventType,
            const primitives::events::ChainEventParams &event) {
          if (auto self = weak.lock()) {
            self->pvf_thread_handler_->execute([weak] {
              if (auto self = weak.lock()) {
                auto r = self->onBlock();
                if (r.has_error()) {
                  SL_DEBUG(self->logger_, "onBlock error {}", r.error());
                }
              }
            });
          }
        });
  }

  outcome::result<void> PvfPrecheck::onBlock() {
    auto block = block_tree_->bestBlock();
    OUTCOME_TRY(signer, signer_factory_->at(block.hash));
    if (not signer.has_value()) {
      return outcome::success();
    }
    if (not session_code_accept_.empty()
        and signer->getSessionIndex() < session_code_accept_.begin()->first) {
      SL_WARN(logger_, "past session");
      return outcome::success();
    }
    auto &session = session_code_accept_[signer->getSessionIndex()];
    OUTCOME_TRY(need, parachain_api_->pvfs_require_precheck(block.hash));
    for (auto &code_hash : need) {
      if (session.contains(code_hash)) {
        continue;
      }
      std::optional<bool> accepted;
      for (auto &p : session_code_accept_) {
        if (auto it = p.second.find(code_hash); it != p.second.end()) {
          accepted = it->second;
          break;
        }
      }
      if (not accepted) {
        auto code_zstd_res =
            parachain_api_->validation_code_by_hash(block.hash, code_hash);
        if (not code_zstd_res or not code_zstd_res.value()) {
          continue;
        }
        auto &code_zstd = *code_zstd_res.value();
        ParachainRuntime code;
        auto timer = metric_pvf_preparation_time.timer();
        auto res = [&]() -> outcome::result<void> {
          OUTCOME_TRY(runtime::uncompressCodeIfNeeded(code_zstd, code));
          OUTCOME_TRY(module_factory_->make(code));
          return outcome::success();
        }();
        timer.reset();
        if (res) {
          SL_VERBOSE(logger_, "approve {}", code_hash);
        } else {
          SL_WARN(logger_, "reject {}: {}", code_hash, res.error());
        }
        accepted = res.has_value();
      }
      PvfCheckStatement statement{
          *accepted,
          code_hash,
          signer->getSessionIndex(),
          signer->validatorIndex(),
      };
      OUTCOME_TRY(signature, signer->signRaw(statement.signable()));
      offchain_worker_pool_->addWorker(offchain_worker_factory_->make());
      ::libp2p::common::FinalAction remove(
          [&] { offchain_worker_pool_->removeWorker(); });
      OUTCOME_TRY(parachain_api_->submit_pvf_check_statement(
          block_tree_->bestBlock().hash, statement, signature));
    }
    while (session_code_accept_.size() > kSessions) {
      session_code_accept_.erase(session_code_accept_.begin());
    }
    return outcome::success();
  }
}  // namespace kagome::parachain
