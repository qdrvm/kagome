/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "scale_codec_impl.hpp"

#include <outcome/outcome.hpp>

#include "primitives/block.hpp"

#include "scale/compact.hpp"

#include "scale/collection.hpp"
#include "scale_codec_impl.hpp"

#include "primitives/scale_error.hpp"

namespace kagome::primitives {

  outcome::result<Buffer> ScaleCodecImpl::encodeBlock(
      const Block &block) const {
    Buffer out;
    OUTCOME_TRY(header, encodeBlockHeader(block.header()));
    out.put(header.toVector());

    for (auto &&extrinsic : block.extrinsics()) {
      OUTCOME_TRY(extrinsic_encode_result, encodeExtrinsic(extrinsic));
      out.put(extrinsic_encode_result.toVector());
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

    collection::encodeCollection(block_header.parentHash().toVector(), out);
    impl::encodeInteger(block_header.number(), out);
    collection::encodeCollection(block_header.stateRoot().toVector(), out);
    collection::encodeCollection(block_header.extrinsicsRoot().toVector(), out);
    collection::encodeCollection(block_header.digest().toVector(), out);

    return out;
  }

  outcome::result<BlockHeader> ScaleCodecImpl::decodeBlockHeader(
      Stream &stream) const {
    auto parent_hash = collection::decodeCollection<uint8_t>(stream);
    if (parent_hash.hasError()) {
      return parent_hash.getError();
    }

    auto number = impl::decodeInteger<size_t>(stream);
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
    auto &&value = extrinsic.getValue();
    compact::encodeInteger(value.size(), out);
    out.put(value);
    return Extrinsic(out);
  }
}  // namespace kagome::primitives
