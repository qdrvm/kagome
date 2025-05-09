/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "parachain/pvf/pvf_impl.hpp"

#include <libp2p/common/shared_fn.hpp>

#include "application/app_configuration.hpp"
#include "application/app_state_manager.hpp"
#include "blockchain/block_tree.hpp"
#include "common/visitor.hpp"
#include "consensus/timeline/timeline.hpp"
#include "log/profiling_logger.hpp"
#include "metrics/histogram_timer.hpp"
#include "parachain/candidate_descriptor_v2.hpp"
#include "parachain/pvf/module_precompiler.hpp"
#include "parachain/pvf/pool.hpp"
#include "parachain/pvf/pvf_error.hpp"
#include "parachain/pvf/pvf_thread_pool.hpp"
#include "parachain/pvf/pvf_worker_types.hpp"
#include "parachain/pvf/session_params.hpp"
#include "parachain/pvf/workers.hpp"
#include "primitives/event_types.hpp"
#include "runtime/common/runtime_execution_error.hpp"
#include "runtime/common/runtime_instances_pool.hpp"
#include "runtime/common/uncompress_code_if_needed.hpp"
#include "runtime/executor.hpp"
#include "runtime/module.hpp"
#include "runtime/module_repository.hpp"
#include "runtime/runtime_code_provider.hpp"
#include "runtime/runtime_context.hpp"
#include "runtime/runtime_instances_pool.hpp"
#include "runtime/wasm_compiler_definitions.hpp"  // this header-file is generated

#define OUTCOME_UNIQUE QTILS_UNIQUE_NAME(outcome)

#define _CB_TRY_VOID(tmp, expr) \
  auto tmp = (expr);            \
  if (tmp.has_error()) {        \
    return cb(tmp.error());     \
  }
#define _CB_TRY_OUT(tmp, out, expr) \
  _CB_TRY_VOID(tmp, expr);          \
  out = std::move(tmp.value());
#define CB_TRYV(expr) _CB_TRY_VOID(OUTCOME_UNIQUE, expr)
#define CB_TRY(out, expr) _CB_TRY_OUT(OUTCOME_UNIQUE, out, expr)

OUTCOME_CPP_DEFINE_CATEGORY(kagome::parachain, PvfError, e) {
  using kagome::parachain::PvfError;
  switch (e) {
    case PvfError::PERSISTED_DATA_HASH:
      return "Incorrect Persisted Data hash";
    case PvfError::NO_CODE:
      return "No code";
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
    case PvfError::COMPILATION_ERROR:
      return "PVF code failed to compile";
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

  // NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
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

  RuntimeEngine pvf_runtime_engine(
      const application::AppConfiguration &app_conf) {
    bool interpreted =
        app_conf.runtimeExecMethod()
        == application::AppConfiguration::RuntimeExecutionMethod::Interpret;

#if KAGOME_WASM_COMPILER_WASM_EDGE == 1
    if (interpreted) {
      // Both Binaryen and WasmEdge could be an interpreter when WasmEdge is
      // compile-enabled
      if (app_conf.runtimeInterpreter()
          == application::AppConfiguration::RuntimeInterpreter::WasmEdge) {
        return RuntimeEngine::kWasmEdgeInterpreted;
      }
      return RuntimeEngine::kBinaryen;
    }
    // Execution method Compiled while WasmEdge is compile-enabled
    return RuntimeEngine::kWasmEdgeCompiled;

#else
    if (interpreted) {  // WasmEdge is compile-disabled
      return RuntimeEngine::kBinaryen;
    }
    return RuntimeEngine::kWAVM;
#endif
  }

  struct ValidationParams {
    HeadData parent_head;
    ParachainBlock block_data;
    BlockNumber relay_parent_number{};
    Hash256 relay_parent_storage_root;
  };

  PvfImpl::PvfImpl(
      const Config &config,
      std::shared_ptr<PvfWorkers> workers,
      std::shared_ptr<crypto::Hasher> hasher,
      std::shared_ptr<PvfPool> pvf_pool,
      std::shared_ptr<blockchain::BlockTree> block_tree,
      std::shared_ptr<crypto::Sr25519Provider> sr25519_provider,
      std::shared_ptr<runtime::ParachainHost> parachain_api,
      std::shared_ptr<runtime::Executor> executor,
      std::shared_ptr<runtime::RuntimeContextFactory> ctx_factory,
      PvfThreadPool &pvf_thread_pool,
      std::shared_ptr<application::AppStateManager> app_state_manager,
      std::shared_ptr<application::AppConfiguration> app_configuration,
      std::shared_ptr<primitives::events::SyncStateSubscriptionEngine>
          sync_state_sub_engine,
      LazySPtr<const consensus::Timeline> timeline)
      : config_{config},
        workers_{std::move(workers)},
        hasher_{std::move(hasher)},
        block_tree_{std::move(block_tree)},
        sr25519_provider_{std::move(sr25519_provider)},
        parachain_api_{std::move(parachain_api)},
        executor_{std::move(executor)},
        ctx_factory_{std::move(ctx_factory)},
        log_{log::createLogger("PVF Executor", "pvf_executor")},
        pvf_pool_{std::move(pvf_pool)},
        precompiler_{std::make_shared<ModulePrecompiler>(
            ModulePrecompiler::Config{
                .precompile_threads_num = config_.precompile_threads_num,
                .opt_level = config_.opt_level},
            parachain_api_,
            pvf_pool_,
            hasher_)},
        pvf_thread_handler_{pvf_thread_pool.handler(*app_state_manager)},
        app_configuration_{std::move(app_configuration)},
        sync_state_sub_engine_{std::move(sync_state_sub_engine)},
        timeline_{timeline} {
    app_state_manager->takeControl(*this);
    constexpr std::array<std::string_view, 4> engines{
        "kBinaryen",
        "kWAVM",
        "kWasmEdgeInterpreted",
        "kWasmEdgeCompiled",
    };
    SL_INFO(log_,
            "pvf runtime engine {}",
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
            engines[fmt::underlying(pvf_runtime_engine(*app_configuration_))]);
    BOOST_ASSERT(sync_state_sub_engine_ != nullptr);
  }

  PvfImpl::~PvfImpl() {
    if (precompiler_thread_) {
      precompiler_thread_->join();
      precompiler_thread_.reset();
    }
  }

  bool PvfImpl::prepare() {
    if (config_.precompile_modules) {
      auto precompiler_bootstrap = [](std::shared_ptr<PvfImpl> self) {
        self->precompiler_thread_ = std::make_unique<std::thread>([self]() {
          soralog::util::setThreadName("pvf_compile");
          SL_DEBUG(self->log_, "Node is synchronized, start precompilation");
          auto res = self->precompiler_->precompileModulesAt(
              self->block_tree_->getLastFinalized().hash);
          if (!res) {
            SL_ERROR(self->log_,
                     "Parachain module precompilation failed: {}",
                     res.error());
          }
        });
      };
      BOOST_ASSERT(timeline_.get() != nullptr);
      if (timeline_.get()->wasSynchronized()) {
        precompiler_bootstrap(shared_from_this());
      } else {
        SL_DEBUG(log_, "Node is not synchronized, delay precompilation");
        sync_state_sub_ = primitives::events::onSync(
            sync_state_sub_engine_,
            [weak = weak_from_this(), precompiler_bootstrap]() {
              BOOST_ASSERT(weak.lock());
              if (auto self = weak.lock()) {
                precompiler_bootstrap(self);
              }
            });
      }
    }
    return true;
  }

  void PvfImpl::pvfValidate(const PersistedValidationData &data,
                            const ParachainBlock &pov,
                            const CandidateReceipt &receipt,
                            const ParachainRuntime &code_zstd,
                            runtime::PvfExecTimeoutKind timeout_kind,
                            Cb cb) const {
    REINVOKE(*pvf_thread_handler_,
             pvfValidate,
             data,
             pov,
             receipt,
             code_zstd,
             timeout_kind,
             std::move(cb));
    // https://github.com/paritytech/polkadot-sdk/blob/1e3b8e1639c1cf784eabf0a9afcab1f3987e0ca4/polkadot/node/core/candidate-validation/src/lib.rs#L763-L782
    auto session = sessionIndex(receipt.descriptor);
    if (session and timeout_kind == runtime::PvfExecTimeoutKind::Backing) {
      CB_TRY(auto expected_session,
             parachain_api_->session_index_for_child(
                 receipt.descriptor.relay_parent));
      if (sessionIndex(receipt.descriptor) != expected_session) {
        cb(network::CheckCoreIndexError::InvalidSession);
        return;
      }
    }

    CB_TRY(auto pov_encoded, scale::encode(pov));
    if (pov_encoded.size() > data.max_pov_size) {
      return cb(PvfError::POV_SIZE);
    }
    auto pov_hash = hasher_->blake2b_256(pov_encoded);
    if (pov_hash != receipt.descriptor.pov_hash) {
      return cb(PvfError::POV_HASH);
    }
    auto code_hash = hasher_->blake2b_256(code_zstd);
    if (code_hash != receipt.descriptor.validation_code_hash) {
      return cb(PvfError::CODE_HASH);
    }
    CB_TRYV(checkSignature(*sr25519_provider_, receipt.descriptor));

    auto timer = metric_pvf_execution_time.timer();
    ValidationParams params;
    params.parent_head = data.parent_head;
    CB_TRYV(runtime::uncompressCodeIfNeeded(pov.payload,
                                            params.block_data.payload));
    params.relay_parent_number = data.relay_parent_number;
    params.relay_parent_storage_root = data.relay_parent_storage_root;
    callWasm(receipt,
             code_hash,
             code_zstd,
             params,
             timeout_kind,
             libp2p::SharedFn{[weak_self{weak_from_this()},
                               data,
                               receipt,
                               timeout_kind,
                               cb{std::move(cb)},
                               timer{std::move(timer)}](
                                  outcome::result<ValidationResult> r) {
               auto self = weak_self.lock();
               if (not self) {
                 return;
               }
               CB_TRY(auto result, std::move(r));
               CB_TRY(auto commitments,
                      self->fromOutputs(receipt, std::move(result)));
               // https://github.com/paritytech/polkadot-sdk/blob/1e3b8e1639c1cf784eabf0a9afcab1f3987e0ca4/polkadot/node/core/candidate-validation/src/lib.rs#L915-L951
               if (timeout_kind == runtime::PvfExecTimeoutKind::Backing
                   and coreIndex(receipt.descriptor)) {
                 CB_TRY(auto claims,
                        self->parachain_api_->claim_queue(
                            receipt.descriptor.relay_parent));
                 if (not claims) {
                   claims.emplace();
                 }
                 CB_TRYV(network::checkCoreIndex(
                     {
                         .descriptor = receipt.descriptor,
                         .commitments = commitments,
                     },
                     transposeClaimQueue(
                         *claims, parachain::DEFAULT_SCHEDULING_LOOKAHEAD)));
               }
               cb(std::make_pair(std::move(commitments), data));
             }});
  }

  void PvfImpl::pvf(const CandidateReceipt &receipt,
                    const ParachainBlock &pov,
                    const runtime::PersistedValidationData &pvd,
                    Cb cb) const {
    REINVOKE(*pvf_thread_handler_, pvf, receipt, pov, pvd, std::move(cb));
    SL_DEBUG(log_,
             "pvf relay_parent={} para_id={}",
             receipt.descriptor.relay_parent,
             receipt.descriptor.para_id);

    auto data_hash = hasher_->blake2b_256(scale::encode(pvd).value());
    if (receipt.descriptor.persisted_data_hash != data_hash) {
      return cb(PvfError::PERSISTED_DATA_HASH);
    }

    CB_TRY(auto code, getCode(receipt.descriptor));
    pvfValidate(pvd,
                pov,
                receipt,
                code,
                runtime::PvfExecTimeoutKind::Backing,
                std::move(cb));
  }

  outcome::result<ParachainRuntime> PvfImpl::getCode(
      const CandidateDescriptor &descriptor) const {
    for (auto assumption : {
             runtime::OccupiedCoreAssumption::Included,
             runtime::OccupiedCoreAssumption::TimedOut,
         }) {
      OUTCOME_TRY(code,
                  parachain_api_->validation_code(
                      descriptor.relay_parent, descriptor.para_id, assumption));
      if (!code) {
        SL_VERBOSE(
            log_,
            "getCode relay_parent={} para_id={}: not found (validation_code)",
            descriptor.relay_parent,
            descriptor.para_id);
        return PvfError::NO_CODE;
      }
      return *code;
    }
    SL_VERBOSE(log_,
               "getCode relay_parent={} para_id={}: not found",
               descriptor.relay_parent,
               descriptor.para_id);
    return PvfError::NO_CODE;
  }

  void PvfImpl::callWasm(const CandidateReceipt &receipt,
                         const common::Hash256 &code_hash,
                         const ParachainRuntime &code_zstd,
                         const ValidationParams &params,
                         runtime::PvfExecTimeoutKind timeout_kind,
                         WasmCb cb) const {
    CB_TRY(auto executor_params,
           sessionParams(*parachain_api_,
                         receipt.descriptor.relay_parent,
                         config_.opt_level));
    const auto &context_params = executor_params.context_params;

    constexpr auto name = "validate_block";
    CB_TRYV(pvf_pool_->precompile(code_hash, code_zstd, context_params));
    if (not app_configuration_->usePvfSubprocess()) {
      // Reusing instances for PVF calls doesn't work, runtime calls start to
      // crash on access out of memory bounds
      KAGOME_PROFILE_START_L(log_, single_process_runtime_instantitation);
      auto module_opt = pvf_pool_->getModule(code_hash, context_params);
      if (!module_opt) {
        SL_ERROR(log_,
                 "Runtime module supposed to be precompiled for parachain ID "
                 "{}, but it's not. This indicates a bug.",
                 receipt.descriptor.para_id);
        cb(PvfError::NO_CODE);
        return;
      }
      auto &wasm_module = module_opt.value();
      CB_TRY(auto instance, wasm_module->instantiate());
      CB_TRY(auto ctx, runtime::RuntimeContextFactory::stateless(instance));
      KAGOME_PROFILE_END(single_process_runtime_instantitation);
      KAGOME_PROFILE_START_L(log_, single_process_runtime_call);
      cb(executor_->call<ValidationResult>(ctx, name, params));
      return;
    }
    kagome::parachain::PvfWorkerInputCodeParams code_params{
        .path = pvf_pool_->getCachePath(code_hash, context_params),
        .context_params = context_params};
    workers_->execute({
        .code_params = std::move(code_params),
        .args = scale::encode(params).value(),
        .cb =
            [cb{std::move(cb)}](outcome::result<common::Buffer> r) {
              if (r.has_error()) {
                cb(r.error());
                return;
              }
              cb(scale::decode<ValidationResult>(r.value()));
            },
        .kind = timeout_kind,
        .timeout =
            std::chrono::milliseconds{
                timeout_kind == runtime::PvfExecTimeoutKind::Backing
                    ? executor_params.pvf_exec_timeout_backing_ms
                    : executor_params.pvf_exec_timeout_approval_ms},
    });
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
    return commitments;
  }
}  // namespace kagome::parachain
