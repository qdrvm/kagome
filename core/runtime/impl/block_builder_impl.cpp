/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/impl/block_builder_impl.hpp"

#include <utility>
#include <vector>

#include "runtime/impl/wasm_memory_stream.hpp"
#include "scale/collection.hpp"
#include "scale/type_decoder.hpp"

namespace kagome::runtime {

  using common::Buffer;
  using extensions::Extension;
  using primitives::Extrinsic;
  using primitives::ScaleCodec;
  using scale::collection::decodeCollection;
  using wasm::Literal;

  BlockBuilderImpl::BlockBuilderImpl(Buffer state_code,
                                     std::shared_ptr<Extension> extension,
                                     std::shared_ptr<ScaleCodec> codec)
      : memory_{extension->memory()},
        codec_{std::move(codec)},
        executor_{std::move(extension)},
        state_code_{std::move(state_code)} {}

  outcome::result<bool> BlockBuilderImpl::apply_extrinsic(
      const Extrinsic &extrinsic) {
    OUTCOME_TRY(encoded_ext, codec_->encodeExtrinsic(extrinsic));

    runtime::SizeType ext_size = encoded_ext.size();
    // TODO (Harrm) PRE-98: after check for memory overflow is done, refactor it
    runtime::WasmPointer ptr = memory_->allocate(ext_size);
    memory_->storeBuffer(ptr, encoded_ext);

    wasm::LiteralList ll{Literal(ptr), Literal(ext_size)};

    OUTCOME_TRY(
        _, executor_.call(state_code_, "BlockBuilder_apply_extrinsic", ll));

    return true;
    // TODO(Harrm) PRE-154 figure out what wasm function returns
  }

  outcome::result<primitives::BlockHeader> BlockBuilderImpl::finalize_block() {
    OUTCOME_TRY(res,
                executor_.call(state_code_, "BlockBuilder_finalize_block", {}));

    uint32_t res_addr = getWasmAddr(res.geti64());

    WasmMemoryStream s(memory_);
    OUTCOME_TRY(s.advance(res_addr));

    OUTCOME_TRY(header, codec_->decodeBlockHeader(s));

    return std::move(header);  // warning from clang-tidy without move
  }

  outcome::result<std::vector<primitives::Extrinsic>>
  BlockBuilderImpl::inherent_extrinsics(const primitives::InherentData &data) {
    OUTCOME_TRY(enc_data, codec_->encodeInherentData(data));

    runtime::SizeType data_size = enc_data.size();
    // TODO (Harrm) PRE-98: after check for memory overflow is done, refactor it
    runtime::WasmPointer ptr = memory_->allocate(data_size);
    memory_->storeBuffer(ptr, enc_data);

    wasm::LiteralList ll{Literal(ptr), Literal(data_size)};

    OUTCOME_TRY(
        res,
        executor_.call(state_code_, "BlockBuilder_inherent_extrinsics", ll));

    uint32_t res_addr = getWasmAddr(res.geti64());

    WasmMemoryStream s(memory_);
    OUTCOME_TRY(s.advance(res_addr));

    auto decode_f = [this](common::ByteStream &stream) {
      return codec_->decodeExtrinsic(stream);
    };
    OUTCOME_TRY(decoded_res, decodeCollection<Extrinsic>(s, decode_f));

    return std::move(decoded_res);
  }

  outcome::result<CheckInherentsResult> BlockBuilderImpl::check_inherents(
      const primitives::Block &block, const primitives::InherentData &data) {
    OUTCOME_TRY(enc_block, codec_->encodeBlock(block));
    OUTCOME_TRY(enc_data, codec_->encodeInherentData(data));

    // TODO (Harrm) PRE-98: after check for memory overflow is done, refactor it
    runtime::SizeType block_size = enc_block.size();
    runtime::WasmPointer block_ptr = memory_->allocate(block_size);
    memory_->storeBuffer(block_ptr, enc_block);

    runtime::SizeType data_size = enc_data.size();
    runtime::WasmPointer data_ptr = memory_->allocate(data_size);
    memory_->storeBuffer(data_ptr, enc_data);

    // TODO(Harrm) PRE-154 check what exactly its input arguments should be
    wasm::LiteralList ll{Literal(data_ptr), Literal(data_size)};

    OUTCOME_TRY(
        res, executor_.call(state_code_, "BlockBuilder_check_inherents", ll));

    uint32_t res_addr = getWasmAddr(res.geti64());

    WasmMemoryStream s(memory_);
    OUTCOME_TRY(s.advance(res_addr));

    scale::TypeDecoder<bool> decoder;
    OUTCOME_TRY(ok, decoder.decode(s));
    OUTCOME_TRY(is_fatal, decoder.decode(s));
    OUTCOME_TRY(error_data, codec_->decodeInherentData(s));

    return CheckInherentsResult{error_data, ok, is_fatal};
  }

  outcome::result<common::Hash256> BlockBuilderImpl::random_seed() {
    // TODO(Harrm) PRE-154 Figure out what it requires
    wasm::LiteralList ll{Literal(0), Literal(0)};
    OUTCOME_TRY(res,
                executor_.call(state_code_, "BlockBuilder_random_seed", ll));

    uint32_t res_addr = getWasmAddr(res.geti64());

    WasmMemoryStream s(memory_);
    OUTCOME_TRY(s.advance(res_addr));

    /// TODO(yuraz): PRE-119 (part of refactor scale)
    common::Hash256 hash;
    for (size_t i = 0; i < common::Hash256::size(); i++) {
      OUTCOME_TRY(byte, scale::fixedwidth::decodeUint8(s));
      hash[i] = byte;
    }

    return hash;
  }

}  // namespace kagome::runtime
