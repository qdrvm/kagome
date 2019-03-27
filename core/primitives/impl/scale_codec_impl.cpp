/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "scale_codec_impl.hpp"

#include "primitives/block.hpp"
#include "scale/collection.hpp"
#include "scale/compact.hpp"
#include "scale/fixedwidth.hpp"
// TODO(yuraz): change path after moving scale_error to scale
#include "primitives/scale_error.hpp"

using namespace kagome::common::scale;

namespace kagome::primitives {

  outcome::result<Buffer> ScaleCodecImpl::encodeBlock(
      const Block &block) const {
    Buffer out;
    OUTCOME_TRY(encoded_header, encodeBlockHeader(block.header()));
    out.putBuffer(encoded_header);

    // put number of extrinsics
    bool res = compact::encodeInteger(block.extrinsics().size(), out);
    // it can't be, res is certainly true
    // this check will disappear after refactoring scale library
    BOOST_ASSERT_MSG(res,
                     "failed to compact-encode collection size");  // NOLINT

    for (auto &&extrinsic : block.extrinsics()) {
      OUTCOME_TRY(encoded_extrinsic, encodeExtrinsic(extrinsic));
      out.putBuffer(encoded_extrinsic);
    }

    return out;
  }

  outcome::result<Block> ScaleCodecImpl::decodeBlock(Stream &stream) const {
    OUTCOME_TRY(header, decodeBlockHeader(stream));
    // decode collection of extrinsics
    // first decode number of items
    auto integer = compact::decodeInteger(stream);
    if (integer.hasError()) {
      return integer.getError();
    }

    auto items_count = integer.getValue().convert_to<uint64_t>();

    std::vector<Extrinsic> collection;
    collection.reserve(items_count);

    for (auto i = 0; i < items_count; ++i) {
      OUTCOME_TRY(extrinsic, decodeExtrinsic(stream));
      collection.push_back(extrinsic);
    }

    return Block(header, collection);
  }

  outcome::result<Buffer> ScaleCodecImpl::encodeBlockHeader(
      const BlockHeader &block_header) const {
    Buffer out;

    collection::encodeBuffer(block_header.parentHash(), out);
    fixedwidth::encodeUint64(block_header.number(), out);
    collection::encodeBuffer(block_header.stateRoot(), out);
    collection::encodeBuffer(block_header.extrinsicsRoot(), out);
    collection::encodeBuffer(block_header.digest(), out);

    return out;
  }

  outcome::result<BlockHeader> ScaleCodecImpl::decodeBlockHeader(
      Stream &stream) const {
    auto parent_hash = collection::decodeCollection<uint8_t>(stream);
    if (parent_hash.hasError()) {
      return parent_hash.getError();
    }

    auto number = fixedwidth::decodeUint64(stream);
    if (!number.has_value()) {
      return DecodeError::kNotEnoughData;
    }

    auto state_root = collection::decodeCollection<uint8_t>(stream);
    if (state_root.hasError()) {
      return state_root.getError();
    }

    auto extrinsics_root = collection::decodeCollection<uint8_t>(stream);
    if (extrinsics_root.hasError()) {
      return extrinsics_root.getError();
    }

    auto digest = collection::decodeCollection<uint8_t>(stream);
    if (digest.hasError()) {
      return digest.getError();
    }

    return BlockHeader(
        Buffer{parent_hash.getValue()}, *number, Buffer{state_root.getValue()},
        Buffer{extrinsics_root.getValue()}, Buffer{digest.getValue()});
  }

  outcome::result<Buffer> ScaleCodecImpl::encodeExtrinsic(
      const Extrinsic &extrinsic) const {
    return extrinsic.data();
  }

  outcome::result<Extrinsic> ScaleCodecImpl::decodeExtrinsic(
      Stream &stream) const {
    auto extrinsic = collection::decodeCollection<uint8_t>(stream);

    Buffer out;

    // extrinsic is an encoded byte array
    auto &&value = extrinsic.getValue();
    // so when we decode it from stream
    // we obtain just byte array
    // and in order to keep its form
    // we need do write its length first
    compact::encodeInteger(value.size(), out);
    // and then bytes as well
    out.put(value);
    return Extrinsic(out);
  }
}  // namespace kagome::primitives
