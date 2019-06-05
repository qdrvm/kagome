/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/impl/block_builder_impl.hpp"

#include <utility>
#include <vector>

#include "scale/scale.hpp"

namespace kagome::runtime {

  using common::Buffer;
  using extensions::Extension;
  using primitives::Block;
  using primitives::BlockHeader;
  using primitives::CheckInherentsResult;
  using primitives::Extrinsic;
  using primitives::InherentData;
  using wasm::Literal;

  BlockBuilderImpl::BlockBuilderImpl(Buffer state_code,
                                     std::shared_ptr<Extension> extension)
      : memory_{extension->memory()},
        executor_{std::move(extension)},
        state_code_{std::move(state_code)} {}

  outcome::result<bool> BlockBuilderImpl::apply_extrinsic(
      const Extrinsic &extrinsic) {
    OUTCOME_TRY(encoded_ext, scale::encode(extrinsic));

    runtime::SizeType ext_size = encoded_ext.size();
    // TODO (Harrm) PRE-98: after check for memory overflow is done, refactor it
    runtime::WasmPointer ptr = memory_->allocate(ext_size);
    memory_->storeBuffer(ptr, common::Buffer(encoded_ext));

    wasm::LiteralList ll{Literal(ptr), Literal(ext_size)};

    OUTCOME_TRY(
        _, executor_.call(state_code_, "BlockBuilder_apply_extrinsic", ll));

    return true;
    // TODO(Harrm) PRE-154 figure out what wasm function returns
  }

  outcome::result<BlockHeader> BlockBuilderImpl::finalize_block() {
    OUTCOME_TRY(res,
                executor_.call(state_code_, "BlockBuilder_finalize_block", {}));

    WasmPointer res_addr = getWasmAddr(res.geti64());
    SizeType len = getWasmLen(res.geti64());
    Buffer buffer = memory_->loadN(res_addr, len);

    return scale::decode<BlockHeader>(buffer);
  }

  outcome::result<std::vector<Extrinsic>> BlockBuilderImpl::inherent_extrinsics(
      const InherentData &data) {
    OUTCOME_TRY(enc_data, scale::encode(data));

    runtime::SizeType data_size = enc_data.size();
    // TODO (Harrm) PRE-98: after check for memory overflow is done, refactor it
    runtime::WasmPointer ptr = memory_->allocate(data_size);
    memory_->storeBuffer(ptr, common::Buffer(std::move(enc_data)));

    wasm::LiteralList ll{Literal(ptr), Literal(data_size)};

    OUTCOME_TRY(
        res,
        executor_.call(state_code_, "BlockBuilder_inherent_extrinsics", ll));

    WasmPointer res_addr = getWasmAddr(res.geti64());
    SizeType len = getWasmLen(res.geti64());
    auto buffer = memory_->loadN(res_addr, len);

    return scale::decode<std::vector<Extrinsic>>(buffer);
  }

  outcome::result<CheckInherentsResult> BlockBuilderImpl::check_inherents(
      const Block &block, const InherentData &data) {
    OUTCOME_TRY(encoded_data, scale::encode(block, data));

    // TODO (Harrm) PRE-98: after check for memory overflow is done, refactor it
    runtime::SizeType param_size = encoded_data.size();
    runtime::WasmPointer param_ptr = memory_->allocate(param_size);
    memory_->storeBuffer(param_ptr, common::Buffer(encoded_data));

    wasm::LiteralList ll{Literal(param_ptr), Literal(param_size)};

    OUTCOME_TRY(
        res, executor_.call(state_code_, "BlockBuilder_check_inherents", ll));

    WasmPointer res_addr = getWasmAddr(res.geti64());
    SizeType len = getWasmLen(res.geti64());

    auto buffer = memory_->loadN(res_addr, len);

    return scale::decode<CheckInherentsResult>(buffer);
  }

  outcome::result<common::Hash256> BlockBuilderImpl::random_seed() {
    // TODO(Harrm) PRE-154 Figure out what it requires
    wasm::LiteralList ll{Literal(0), Literal(0)};
    OUTCOME_TRY(res,
                executor_.call(state_code_, "BlockBuilder_random_seed", ll));

    WasmPointer res_addr = getWasmAddr(res.geti64());
    SizeType len = getWasmLen(res.geti64());
    auto buffer = memory_->loadN(res_addr, len);

    return scale::decode<common::Hash256>(buffer);
  }

}  // namespace kagome::runtime
