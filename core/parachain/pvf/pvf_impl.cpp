/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "parachain/pvf/pvf_impl.hpp"

#include "runtime/common/executor.hpp"
#include "runtime/common/uncompress_code_if_needed.hpp"
#include "runtime/module.hpp"
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
  return fmt::format("PvfError({})", e);
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
        std::shared_ptr<const runtime::RuntimeCodeProvider>,
        const primitives::BlockInfo &,
        const primitives::BlockHeader &) override {
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

  struct ValidationResult {
    SCALE_TIE(6);

    HeadData head_data;
    std::optional<ParachainRuntime> new_validation_code;
    std::vector<UpwardMessage> upward_messages;
    std::vector<OutboundHorizontal> horizontal_messages;
    uint32_t processed_downward_messages;
    BlockNumber hrmp_watermark;
  };

  PvfImpl::PvfImpl(
      std::shared_ptr<crypto::Hasher> hasher,
      std::shared_ptr<runtime::ModuleFactory> module_factory,
      std::shared_ptr<runtime::RuntimePropertiesCache> runtime_properties_cache,
      std::shared_ptr<blockchain::BlockHeaderRepository>
          block_header_repository,
      std::shared_ptr<crypto::Sr25519Provider> sr25519_provider,
      std::shared_ptr<runtime::ParachainHost> parachain_api)
      : hasher_{std::move(hasher)},
        module_factory_{std::move(module_factory)},
        runtime_properties_cache_{std::move(runtime_properties_cache)},
        block_header_repository_{std::move(block_header_repository)},
        sr25519_provider_{std::move(sr25519_provider)},
        parachain_api_{std::move(parachain_api)},
        log_{log::createLogger("Pvf")} {}

  outcome::result<Pvf::Result> PvfImpl::pvfSync(
      const CandidateReceipt &receipt, const ParachainBlock &pov) const {
    SL_DEBUG(log_,
             "pvfSync relay_parent={} para_id={}",
             receipt.descriptor.relay_parent,
             receipt.descriptor.para_id);
    OUTCOME_TRY(data_code, findData(receipt.descriptor));
    auto &[data, code] = data_code;
    OUTCOME_TRY(pov_encoded, scale::encode(pov));
    if (pov_encoded.size() > data.max_pov_size) {
      return PvfError::POV_SIZE;
    }
    auto pov_hash = hasher_->blake2b_256(pov_encoded);
    if (pov_hash != receipt.descriptor.pov_hash) {
      return PvfError::POV_HASH;
    }
    auto code_hash = hasher_->blake2b_256(code);
    if (code_hash != receipt.descriptor.para_runtime_hash) {
      return PvfError::CODE_HASH;
    }
    OUTCOME_TRY(signature_valid,
                sr25519_provider_->verify(receipt.descriptor.signature,
                                          receipt.descriptor.signable(),
                                          receipt.descriptor.collator_id));
    if (!signature_valid) {
      return PvfError::SIGNATURE;
    }

    ValidationParams params;
    params.parent_head = data.parent_head;
    OUTCOME_TRY(runtime::uncompressCodeIfNeeded(pov.payload,
                                                params.block_data.payload));
    params.relay_parent_number = data.relay_parent_number;
    params.relay_parent_storage_root = data.relay_parent_storage_root;
    OUTCOME_TRY(result, callWasm(code, params));

    OUTCOME_TRY(commitments, fromOutputs(receipt, std::move(result)));
    return std::make_pair(std::move(commitments), std::move(data));
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
      const ParachainRuntime &code_zstd, const ValidationParams &params) const {
    ParachainRuntime code;
    OUTCOME_TRY(runtime::uncompressCodeIfNeeded(code_zstd, code));
    OUTCOME_TRY(module, module_factory_->make(code));
    OUTCOME_TRY(instance, module->instantiate());
    auto env_factory = std::make_shared<runtime::RuntimeEnvironmentFactory>(
        std::make_shared<DontProvideCode>(),
        std::make_shared<ReturnModuleInstance>(instance),
        block_header_repository_);
    auto executor = std::make_unique<runtime::Executor>(
        env_factory, runtime_properties_cache_);
    return executor->callAtGenesis<ValidationResult>("validate_block", params);
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
