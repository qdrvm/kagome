/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "parachain/pvf/precheck.hpp"

#include <libp2p/common/final_action.hpp>

#include "blockchain/block_tree.hpp"
#include "offchain/offchain_worker_factory.hpp"
#include "offchain/offchain_worker_pool.hpp"
#include "parachain/pvf/pool.hpp"
#include "parachain/pvf/pvf_thread_pool.hpp"
#include "parachain/pvf/session_params.hpp"
#include "runtime/common/runtime_instances_pool.hpp"
#include "runtime/module.hpp"

namespace kagome::parachain {
  constexpr size_t kSessions = 3;

  PvfPrecheck::PvfPrecheck(
      std::shared_ptr<crypto::Hasher> hasher,
      std::shared_ptr<blockchain::BlockTree> block_tree,
      std::shared_ptr<ValidatorSignerFactory> signer_factory,
      std::shared_ptr<runtime::ParachainHost> parachain_api,
      std::shared_ptr<PvfPool> pvf_pool,
      std::shared_ptr<runtime::Executor> executor,
      PvfThreadPool &pvf_thread_pool,
      std::shared_ptr<offchain::OffchainWorkerFactory> offchain_worker_factory,
      std::shared_ptr<offchain::OffchainWorkerPool> offchain_worker_pool,
      primitives::events::ChainSubscriptionEnginePtr chain_sub_engine)
      : hasher_{std::move(hasher)},
        block_tree_{std::move(block_tree)},
        signer_factory_{std::move(signer_factory)},
        parachain_api_{std::move(parachain_api)},
        pvf_pool_{std::move(pvf_pool)},
        executor_{std::move(executor)},
        offchain_worker_factory_{std::move(offchain_worker_factory)},
        offchain_worker_pool_{std::move(offchain_worker_pool)},
        chain_sub_{std::move(chain_sub_engine)},
        pvf_thread_handler_{pvf_thread_pool.handlerManual()} {
    BOOST_ASSERT(hasher_ != nullptr);
    BOOST_ASSERT(block_tree_ != nullptr);
    BOOST_ASSERT(signer_factory_ != nullptr);
    BOOST_ASSERT(parachain_api_ != nullptr);
    BOOST_ASSERT(executor_ != nullptr);
    BOOST_ASSERT(offchain_worker_factory_ != nullptr);
    BOOST_ASSERT(offchain_worker_pool_ != nullptr);
  }

  void PvfPrecheck::start() {
    pvf_thread_handler_->start();

    chain_sub_.onHead([weak{weak_from_this()}] {
      if (auto self = weak.lock()) {
        self->pvf_thread_handler_->execute([weak] {
          if (auto self = weak.lock()) {
            auto res = self->onBlock();
            if (res.has_error()) {
              SL_DEBUG(self->logger_, "onBlock error {}", res.error());
            }
          }
        });
      }
    });
  }

  outcome::result<void> PvfPrecheck::onBlock() {
    auto block = block_tree_->bestBlock();
    OUTCOME_TRY(opt_signer, signer_factory_->at(block.hash));
    if (!opt_signer) {
      return outcome::success();
    }
    auto &signer = *opt_signer;

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
      for (auto &code_hashes : session_code_accept_ | std::views::values) {
        if (auto it = code_hashes.find(code_hash); it != code_hashes.end()) {
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
        auto res = [&]() -> outcome::result<void> {
          OUTCOME_TRY(config, sessionParams(*parachain_api_, block.hash));
          OUTCOME_TRY(pvf_pool_->precompile(
              code_hash, code_zstd, config.context_params));
          return outcome::success();
        }();
        if (res) {
          SL_VERBOSE(logger_, "approve {}", code_hash);
        } else {
          SL_WARN(logger_, "reject {}: {}", code_hash, res.error());
        }
        accepted = res.has_value();
      }
      PvfCheckStatement statement{
          .accept = accepted.value(),
          .subject = code_hash,
          .session_index = signer->getSessionIndex(),
          .validator_index = signer->validatorIndex(),
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
