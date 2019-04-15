/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "primitives/impl/scale_codec_impl.hpp"

#include "primitives/block.hpp"
#include "scale/collection.hpp"
#include "scale/compact.hpp"
#include "scale/fixedwidth.hpp"
#include "scale/scale_error.hpp"

using kagome::common::Buffer;
using namespace kagome::scale;  // NOLINT

namespace kagome::primitives {

  outcome::result<Buffer> ScaleCodecImpl::encodeBlock(
      const Block &block) const {
    Buffer out;
    OUTCOME_TRY(encoded_header, encodeBlockHeader(block.header()));
    out.putBuffer(encoded_header);

    // put number of extrinsics
    OUTCOME_TRY(compact::encodeInteger(block.extrinsics().size(), out));

    for (auto &&extrinsic : block.extrinsics()) {
      OUTCOME_TRY(encoded_extrinsic, encodeExtrinsic(extrinsic));
      out.putBuffer(encoded_extrinsic);
    }

    return out;
  }

  outcome::result<Block> ScaleCodecImpl::decodeBlock(Stream &stream) const {
    OUTCOME_TRY(header, decodeBlockHeader(stream));
    // first decode number of items
    OUTCOME_TRY(integer, compact::decodeInteger(stream));
    auto items_count = integer.convert_to<uint64_t>();

    std::vector<Extrinsic> collection;
    collection.reserve(items_count);

    // now decode collection of extrinsics
    for (auto i = 0; i < items_count; ++i) {
      OUTCOME_TRY(extrinsic, decodeExtrinsic(stream));
      collection.push_back(extrinsic);
    }

    return Block(header, collection);
  }

  outcome::result<Buffer> ScaleCodecImpl::encodeBlockHeader(
      const BlockHeader &block_header) const {
    Buffer out;

    OUTCOME_TRY(collection::encodeBuffer(block_header.parentHash(), out));
    fixedwidth::encodeUint64(block_header.number(), out);
    OUTCOME_TRY(collection::encodeBuffer(block_header.stateRoot(), out));
    OUTCOME_TRY(collection::encodeBuffer(block_header.extrinsicsRoot(), out));
    OUTCOME_TRY(collection::encodeBuffer(block_header.digest(), out));

    return out;
  }

  outcome::result<BlockHeader> ScaleCodecImpl::decodeBlockHeader(
      Stream &stream) const {
    OUTCOME_TRY(parent_hash, collection::decodeCollection<uint8_t>(stream));
    OUTCOME_TRY(number, fixedwidth::decodeUint64(stream));
    OUTCOME_TRY(state_root, collection::decodeCollection<uint8_t>(stream));
    OUTCOME_TRY(extrinsics_root, collection::decodeCollection<uint8_t>(stream));
    OUTCOME_TRY(digest, collection::decodeCollection<uint8_t>(stream));

    return BlockHeader(Buffer{parent_hash}, number, Buffer{state_root},
                       Buffer{extrinsics_root}, Buffer{digest});
  }

  outcome::result<Buffer> ScaleCodecImpl::encodeExtrinsic(
      const Extrinsic &extrinsic) const {
    return extrinsic.data();
  }

  outcome::result<Extrinsic> ScaleCodecImpl::decodeExtrinsic(
      Stream &stream) const {
    Buffer out;

    OUTCOME_TRY(extrinsic, collection::decodeCollection<uint8_t>(stream));
    // extrinsic is an encoded byte array, so when we decode it from stream
    // we obtain just byte array, and in order to keep its form
    // we need do write its size first
    OUTCOME_TRY(compact::encodeInteger(extrinsic.size(), out));
    // and then bytes as well
    out.put(extrinsic);

    return Extrinsic(out);
  }
}  // namespace kagome::primitives
