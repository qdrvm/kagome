/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "parachain/pvf/pvf_impl.hpp"

#include "application/app_configuration.hpp"
#include "application/app_state_manager.hpp"
#include "blockchain/block_tree.hpp"
#include "common/visitor.hpp"
#include "metrics/histogram_timer.hpp"
#include "runtime/common/runtime_instances_pool.hpp"
#include "runtime/common/uncompress_code_if_needed.hpp"
#include "runtime/executor.hpp"
#include "runtime/module.hpp"
#include "runtime/module_factory.hpp"
#include "runtime/module_repository.hpp"
#include "runtime/runtime_code_provider.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::parachain, PvfError, e) {
  using kagome::parachain::PvfError;
  switch (e) {
    case PvfError::NO_PERSISTED_DATA:
      return "PersistedValidationData was not found";
    case PvfError::POV_SIZE:
      return "PoV is too big";
    case PvfError::POV_HASH:
      return "PoV hash mismatch";
    case PvfError::CODE_HASH:
      return "Code hash mismatch";
    case PvfError::SIGNATURE:
      return "Signature is invalid";
    case PvfError::HEAD_HASH:
      return "Head hash mismatch";
    case PvfError::COMMITMENTS_HASH:
      return "Commitments hash mismatch";
    case PvfError::OUTPUTS:
      return "ValidationResult is invalid";
  }
  return "unknown error (kagome::parachain::PvfError)";
}

namespace kagome::parachain {
  using common::Hash256;
  using network::HeadData;
  using network::OutboundHorizontal;
  using network::ParachainBlock;
  using network::ParachainRuntime;
  using network::UpwardMessage;
  using primitives::BlockNumber;
  using runtime::PersistedValidationData;

  metrics::HistogramTimer metric_pvf_execution_time{
      "kagome_pvf_execution_time",
      "Time spent in executing PVFs",
      {
          0.01,
          0.025,
          0.05,
          0.1,
          0.25,
          0.5,
          1.0,
          2.0,
          3.0,
          4.0,
          5.0,
          6.0,
          8.0,
          10.0,
          12.0,
      },
  };

  metrics::HistogramHelper metric_code_size{
      "kagome_parachain_candidate_validation_code_size",
      "The size of the decompressed WASM validation blob used for checking a "
      "candidate",
      metrics::exponentialBuckets(16384, 2, 10),
  };

  struct DontProvideCode : runtime::RuntimeCodeProvider {
    outcome::result<gsl::span<const uint8_t>> getCodeAt(
        const storage::trie::RootHash &) const override {
      abort();
    }
  };

  struct ReturnModuleInstance : runtime::ModuleRepository {
    ReturnModuleInstance(std::shared_ptr<runtime::ModuleInstance> instance)
        : instance{std::move(instance)} {}

    outcome::result<std::shared_ptr<runtime::ModuleInstance>> getInstanceAt(
        const primitives::BlockInfo &,
        const storage::trie::RootHash &) override {
      return instance;
    }

    std::shared_ptr<runtime::ModuleInstance> instance;
  };

  struct ValidationParams {
    SCALE_TIE(4);

    HeadData parent_head;
    ParachainBlock block_data;
    BlockNumber relay_parent_number;
    Hash256 relay_parent_storage_root;
  };

  PvfImpl::PvfImpl(
      std::shared_ptr<crypto::Hasher> hasher,
      std::shared_ptr<runtime::ModuleFactory> module_factory,
      std::shared_ptr<runtime::RuntimePropertiesCache> runtime_properties_cache,
      std::shared_ptr<blockchain::BlockTree> block_tree,
      std::shared_ptr<crypto::Sr25519Provider> sr25519_provider,
      std::shared_ptr<runtime::ParachainHost> parachain_api,
      std::shared_ptr<runtime::Executor> executor,
      std::shared_ptr<runtime::RuntimeContextFactory> ctx_factory,
      std::shared_ptr<application::AppConfiguration> config,
      std::shared_ptr<application::AppStateManager> state_manager)
      : hasher_{std::move(hasher)},
        runtime_properties_cache_{std::move(runtime_properties_cache)},
        block_tree_{std::move(block_tree)},
        sr25519_provider_{std::move(sr25519_provider)},
        parachain_api_{std::move(parachain_api)},
        executor_{std::move(executor)},
        ctx_factory_{std::move(ctx_factory)},
        log_{log::createLogger("PVF Executor", "parachain")},
        runtime_cache_{std::make_shared<runtime::RuntimeInstancesPool>(
            module_factory, config->parachainRuntimeInstanceCacheSize())} {
    state_manager->takeControl(*this);
  }

  bool PvfImpl::prepare() {
    auto prepare = [this]() -> outcome::result<void> {
      auto last_finalized = block_tree_->getLastFinalized();
      SL_DEBUG(log_,
               "Warming up PVF executor runtime instance cache at block {}",
               last_finalized);
      OUTCOME_TRY(cores,
                  parachain_api_->availability_cores(last_finalized.hash));
      for (auto &core : cores) {
        // empty core
        if (std::holds_alternative<runtime::EmptyCore>(core)) {
          continue;
        }

        OUTCOME_TRY(para_id,
                    visit_in_place(
                        core,
                        [this](runtime::OccupiedCore const &core) {
                          SL_TRACE(log_, "Occupied availability core");
                          return core.candidate_descriptor.para_id;
                        },
                        [this](runtime::ScheduledCore const &core) {
                          SL_TRACE(log_, "Scheduled availability core");
                          return core.para_id;
                        },
                        [](runtime::EmptyCore) -> outcome::result<ParachainId> {
                          BOOST_UNREACHABLE_RETURN({})
                        }));
        // TODO: Is the assumption correct?
        OUTCOME_TRY(code_opt,
                    parachain_api_->validation_code(
                        last_finalized.hash,
                        para_id,
                        runtime::OccupiedCoreAssumption::Included));
        // TODO: Under what circumstances can this happen?
        if (!code_opt) {
          SL_DEBUG(log_,
                   "No validation code found for parachain {} with 'included' "
                   "occupied assumption",
                   para_id);
          continue;
        }
        auto &code = *code_opt;
        auto hash = hasher_->blake2b_256(code);
        OUTCOME_TRY(runtime_cache_->instantiate(hash, code));
        SL_DEBUG(
            log_,
            "Instantiated runtime instance with code hash {} for parachain {}",
            hash,
            para_id);
      }
      return outcome::success();
    };
    if (auto res = prepare(); !res) {
      SL_ERROR(log_, "Failed to initialize PVF executor: {}", res.error());
      return false;
    }
    return true;
  }

  outcome::result<Pvf::Result> PvfImpl::pvfValidate(
      const PersistedValidationData &data,
      const ParachainBlock &pov,
      const CandidateReceipt &receipt,
      const ParachainRuntime &code_zstd) const {
    OUTCOME_TRY(pov_encoded, scale::encode(pov));
    if (pov_encoded.size() > data.max_pov_size) {
      return PvfError::POV_SIZE;
    }
    auto pov_hash = hasher_->blake2b_256(pov_encoded);
    if (pov_hash != receipt.descriptor.pov_hash) {
      return PvfError::POV_HASH;
    }
    auto code_hash = hasher_->blake2b_256(code_zstd);
    if (code_hash != receipt.descriptor.validation_code_hash) {
      return PvfError::CODE_HASH;
    }
    OUTCOME_TRY(signature_valid,
                sr25519_provider_->verify(receipt.descriptor.signature,
                                          receipt.descriptor.signable(),
                                          receipt.descriptor.collator_id));
    if (!signature_valid) {
      return PvfError::SIGNATURE;
    }

    auto timer = metric_pvf_execution_time.timer();
    ParachainRuntime code;
    OUTCOME_TRY(runtime::uncompressCodeIfNeeded(code_zstd, code));
    metric_code_size.observe(code.size());
    ValidationParams params;
    params.parent_head = data.parent_head;
    OUTCOME_TRY(runtime::uncompressCodeIfNeeded(pov.payload,
                                                params.block_data.payload));
    params.relay_parent_number = data.relay_parent_number;
    params.relay_parent_storage_root = data.relay_parent_storage_root;
    OUTCOME_TRY(result, callWasm(receipt, code_hash, code, params));
    timer.reset();

    OUTCOME_TRY(commitments, fromOutputs(receipt, std::move(result)));
    return std::make_pair(std::move(commitments), std::move(data));
  }

  outcome::result<Pvf::Result> PvfImpl::pvfSync(
      const CandidateReceipt &receipt, const ParachainBlock &pov) const {
    SL_DEBUG(log_,
             "pvfSync relay_parent={} para_id={}",
             receipt.descriptor.relay_parent,
             receipt.descriptor.para_id);
    OUTCOME_TRY(data_code, findData(receipt.descriptor));
    auto &[data, code] = data_code;
    return pvfValidate(data, pov, receipt, code);
  }

  outcome::result<std::pair<PersistedValidationData, ParachainRuntime>>
  PvfImpl::findData(const CandidateDescriptor &descriptor) const {
    for (auto assumption : {
             runtime::OccupiedCoreAssumption::Included,
             runtime::OccupiedCoreAssumption::TimedOut,
         }) {
      OUTCOME_TRY(data,
                  parachain_api_->persisted_validation_data(
                      descriptor.relay_parent, descriptor.para_id, assumption));
      if (!data) {
        SL_VERBOSE(log_,
                   "findData relay_parent={} para_id={}: not found "
                   "(persisted_validation_data)",
                   descriptor.relay_parent,
                   descriptor.para_id);
        return PvfError::NO_PERSISTED_DATA;
      }
      auto data_hash = hasher_->blake2b_256(scale::encode(*data).value());
      if (descriptor.persisted_data_hash != data_hash) {
        continue;
      }
      OUTCOME_TRY(code,
                  parachain_api_->validation_code(
                      descriptor.relay_parent, descriptor.para_id, assumption));
      if (!code) {
        SL_VERBOSE(
            log_,
            "findData relay_parent={} para_id={}: not found (validation_code)",
            descriptor.relay_parent,
            descriptor.para_id);
        return PvfError::NO_PERSISTED_DATA;
      }
      return std::make_pair(*data, *code);
    }
    SL_VERBOSE(log_,
               "findData relay_parent={} para_id={}: not found",
               descriptor.relay_parent,
               descriptor.para_id);
    return PvfError::NO_PERSISTED_DATA;
  }

  outcome::result<ValidationResult> PvfImpl::callWasm(
      const CandidateReceipt &receipt,
      const common::Hash256 &code_hash,
      const ParachainRuntime &code_zstd,
      const ValidationParams &params) const {
    OUTCOME_TRY(instance, runtime_cache_->instantiate(code_hash, code_zstd));

    runtime::RuntimeContext::ContextParams executor_params{};
    auto &parent_hash = receipt.descriptor.relay_parent;
    OUTCOME_TRY(session_index,
                parachain_api_->session_index_for_child(parent_hash));
    OUTCOME_TRY(
        session_params,
        parachain_api_->session_executor_params(parent_hash, session_index));
    OUTCOME_TRY(ctx,
                ctx_factory_->ephemeral(
                    instance, storage::trie::kEmptyRootHash, executor_params));
    return executor_->decodedCallWithCtx<ValidationResult>(
        ctx, "validate_block", params);
  }

  outcome::result<Pvf::CandidateCommitments> PvfImpl::fromOutputs(
      const CandidateReceipt &receipt, ValidationResult &&result) const {
    auto head_hash = hasher_->blake2b_256(result.head_data);
    if (head_hash != receipt.descriptor.para_head_hash) {
      return PvfError::HEAD_HASH;
    }
    CandidateCommitments commitments{
        .upward_msgs = std::move(result.upward_messages),
        .outbound_hor_msgs = std::move(result.horizontal_messages),
        .opt_para_runtime = std::move(result.new_validation_code),
        .para_head = std::move(result.head_data),
        .downward_msgs_count = result.processed_downward_messages,
        .watermark = result.hrmp_watermark,
    };
    auto commitments_hash =
        hasher_->blake2b_256(scale::encode(commitments).value());
    if (commitments_hash != receipt.commitments_hash) {
      return PvfError::COMMITMENTS_HASH;
    }
    OUTCOME_TRY(valid,
                parachain_api_->check_validation_outputs(
                    receipt.descriptor.relay_parent,
                    receipt.descriptor.para_id,
                    commitments));
    if (!valid) {
      SL_VERBOSE(log_,
                 "fromOutputs relay_parent={} para_id={}: invalid "
                 "(check_validation_outputs)",
                 receipt.descriptor.relay_parent,
                 receipt.descriptor.para_id);
      return PvfError::OUTPUTS;
    }
    return commitments;
  }
}  // namespace kagome::parachain
