/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "parachain/pvf/pvf.hpp"

#include <thread>

#include "crypto/sr25519_provider.hpp"
#include "injector/lazy.hpp"
#include "log/logger.hpp"
#include "primitives/event_types.hpp"
#include "runtime/runtime_api/parachain_host.hpp"

namespace kagome {
  class PoolHandler;

  namespace consensus {
    class Timeline;
  }
}  // namespace kagome

namespace kagome::application {
  class AppConfiguration;
  class AppStateManager;
}  // namespace kagome::application

namespace kagome::blockchain {
  class BlockTree;
}

namespace kagome::runtime {
  class Executor;
  class RuntimeContextFactory;
  class RuntimeInstancesPool;
}  // namespace kagome::runtime

namespace kagome::parachain {
  class PvfPool;
  class PvfThreadPool;
  class PvfWorkers;

  class ModulePrecompiler;

  struct ValidationParams;

  struct ValidationResult {
    HeadData head_data;
    std::optional<ParachainRuntime> new_validation_code;
    std::vector<UpwardMessage> upward_messages;
    std::vector<network::OutboundHorizontal> horizontal_messages;
    uint32_t processed_downward_messages{};
    BlockNumber hrmp_watermark{};
  };

  class PvfImpl : public Pvf, public std::enable_shared_from_this<PvfImpl> {
   public:
    struct Config {
      bool precompile_modules;
      unsigned precompile_threads_num{1};
    };

    PvfImpl(const Config &config,
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
            LazySPtr<const consensus::Timeline> timeline);

    ~PvfImpl() override;

    bool prepare();

    void pvf(const CandidateReceipt &receipt,
             const ParachainBlock &pov,
             const runtime::PersistedValidationData &pvd,
             Cb cb) const override;
    void pvfValidate(const PersistedValidationData &data,
                     const ParachainBlock &pov,
                     const CandidateReceipt &receipt,
                     const ParachainRuntime &code,
                     runtime::PvfExecTimeoutKind timeout_kind,
                     Cb cb) const override;

   private:
    using CandidateDescriptor = network::CandidateDescriptor;
    using ParachainRuntime = network::ParachainRuntime;
    using WasmCb = std::function<void(outcome::result<ValidationResult>)>;

    outcome::result<ParachainRuntime> getCode(
        const CandidateDescriptor &descriptor) const;
    void callWasm(const CandidateReceipt &receipt,
                  const common::Hash256 &code_hash,
                  const ParachainRuntime &code_zstd,
                  const ValidationParams &params,
                  runtime::PvfExecTimeoutKind timeout_kind,
                  WasmCb cb) const;

    outcome::result<CandidateCommitments> fromOutputs(
        const CandidateReceipt &receipt, ValidationResult &&result) const;

    Config config_;
    std::shared_ptr<PvfWorkers> workers_;
    std::shared_ptr<crypto::Hasher> hasher_;
    std::shared_ptr<blockchain::BlockTree> block_tree_;
    std::shared_ptr<crypto::Sr25519Provider> sr25519_provider_;
    std::shared_ptr<runtime::ParachainHost> parachain_api_;
    std::shared_ptr<runtime::Executor> executor_;
    std::shared_ptr<runtime::RuntimeContextFactory> ctx_factory_;
    log::Logger log_;

    std::shared_ptr<PvfPool> pvf_pool_;
    std::shared_ptr<ModulePrecompiler> precompiler_;
    std::shared_ptr<PoolHandler> pvf_thread_handler_;
    std::shared_ptr<application::AppConfiguration> app_configuration_;
    std::shared_ptr<primitives::events::SyncStateSubscriptionEngine>
        sync_state_sub_engine_;
    std::shared_ptr<void> sync_state_sub_;
    std::unique_ptr<std::thread> precompiler_thread_;
    LazySPtr<const consensus::Timeline> timeline_;
  };
}  // namespace kagome::parachain
