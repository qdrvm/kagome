/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_PARACHAIN_PVF_PVF_IMPL_HPP
#define KAGOME_PARACHAIN_PVF_PVF_IMPL_HPP

#include "parachain/pvf/pvf.hpp"

#include <shared_mutex>
#include <thread>

#include "blockchain/block_header_repository.hpp"
#include "crypto/sr25519_provider.hpp"
#include "log/logger.hpp"
#include "runtime/runtime_api/parachain_host.hpp"
#include "runtime/runtime_properties_cache.hpp"

namespace kagome::runtime {
  class ModuleInstance;
  class ModuleFactory;
  class Executor;
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
  struct ValidationResult;

  class PvfImpl : public Pvf {
   public:
    struct Config {
      uint64_t instance_cache_size;
    };
    PvfImpl(std::shared_ptr<crypto::Hasher> hasher,
            std::shared_ptr<runtime::ModuleFactory> module_factory,
            std::shared_ptr<runtime::RuntimePropertiesCache>
                runtime_properties_cache,
            std::shared_ptr<blockchain::BlockHeaderRepository>
                block_header_repository,
            std::shared_ptr<crypto::Sr25519Provider> sr25519_provider,
            std::shared_ptr<runtime::ParachainHost> parachain_api,
            const Config& config);
    ~PvfImpl() override;

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
        ParachainId para_id,
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
    log::Logger log_;

    std::unique_ptr<class PvfRuntimeCache> runtime_cache_;
  };
}  // namespace kagome::parachain

#endif  // KAGOME_PARACHAIN_PVF_PVF_IMPL_HPP
