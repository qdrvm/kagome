/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_PARACHAIN_PVF_PVF_IMPL_HPP
#define KAGOME_PARACHAIN_PVF_PVF_IMPL_HPP

#include "parachain/pvf/pvf.hpp"

#include "blockchain/block_header_repository.hpp"
#include "crypto/sr25519_provider.hpp"
#include "log/logger.hpp"
#include "runtime/runtime_api/parachain_host.hpp"
#include "runtime/runtime_properties_cache.hpp"

namespace kagome::application {
  class AppConfiguration;
}

namespace kagome::runtime {
  class ModuleInstance;
  class ModuleFactory;
  class Executor;
  class RuntimeContextFactory;
  class RuntimeInstancesPool;
}  // namespace kagome::runtime

namespace kagome::parachain {
  enum class PvfError {
    // NO_DATA conflicted with <netdb.h>
    NO_PERSISTED_DATA = 1,
    POV_SIZE,
    POV_HASH,
    CODE_HASH,
    SIGNATURE,
    HEAD_HASH,
    COMMITMENTS_HASH,
    OUTPUTS,
  };
}  // namespace kagome::parachain

OUTCOME_HPP_DECLARE_ERROR(kagome::parachain, PvfError)

namespace kagome::parachain {
  struct ValidationParams;

  struct ValidationResult {
    SCALE_TIE(6);

    HeadData head_data;
    std::optional<ParachainRuntime> new_validation_code;
    std::vector<UpwardMessage> upward_messages;
    std::vector<network::OutboundHorizontal> horizontal_messages;
    uint32_t processed_downward_messages;
    BlockNumber hrmp_watermark;
  };

  class PvfImpl : public Pvf {
   public:
    PvfImpl(std::shared_ptr<crypto::Hasher> hasher,
            std::shared_ptr<runtime::ModuleFactory> module_factory,
            std::shared_ptr<runtime::RuntimePropertiesCache>
                runtime_properties_cache,
            std::shared_ptr<blockchain::BlockHeaderRepository>
                block_header_repository,
            std::shared_ptr<crypto::Sr25519Provider> sr25519_provider,
            std::shared_ptr<runtime::ParachainHost> parachain_api,
            std::shared_ptr<runtime::Executor> executor,
            std::shared_ptr<runtime::RuntimeContextFactory> ctx_factory,
            std::shared_ptr<application::AppConfiguration> config);

    outcome::result<Result> pvfSync(const CandidateReceipt &receipt,
                                    const ParachainBlock &pov) const override;
    outcome::result<Result> pvfValidate(
        const PersistedValidationData &data,
        const ParachainBlock &pov,
        const CandidateReceipt &receipt,
        const ParachainRuntime &code) const override;

   private:
    using CandidateDescriptor = network::CandidateDescriptor;
    using ParachainRuntime = network::ParachainRuntime;

    outcome::result<std::pair<PersistedValidationData, ParachainRuntime>>
    findData(const CandidateDescriptor &descriptor) const;
    outcome::result<ValidationResult> callWasm(
        const CandidateReceipt &receipt,
        const common::Hash256 &code_hash,
        const ParachainRuntime &code_zstd,
        const ValidationParams &params) const;
    outcome::result<CandidateCommitments> fromOutputs(
        const CandidateReceipt &receipt, ValidationResult &&result) const;

    std::shared_ptr<crypto::Hasher> hasher_;
    std::shared_ptr<runtime::RuntimePropertiesCache> runtime_properties_cache_;
    std::shared_ptr<blockchain::BlockHeaderRepository> block_header_repository_;
    std::shared_ptr<crypto::Sr25519Provider> sr25519_provider_;
    std::shared_ptr<runtime::ParachainHost> parachain_api_;
    std::shared_ptr<runtime::Executor> executor_;
    std::shared_ptr<runtime::RuntimeContextFactory> ctx_factory_;
    log::Logger log_;

    std::shared_ptr<runtime::RuntimeInstancesPool> runtime_cache_;
  };
}  // namespace kagome::parachain

#endif  // KAGOME_PARACHAIN_PVF_PVF_IMPL_HPP
