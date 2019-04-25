/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "primitives/impl/scale_codec_impl.hpp"

#include "primitives/block.hpp"
#include "primitives/block_id.hpp"
#include "primitives/transaction_validity.hpp"
#include "primitives/version.hpp"
#include "scale/collection.hpp"
#include "scale/compact.hpp"
#include "scale/fixedwidth.hpp"
#include "scale/scale_error.hpp"
#include "scale/variant.hpp"

/// custom types encoders and decoders
namespace kagome::scale {
  /// encodes std::array
  template <class T, size_t sz>
  struct TypeEncoder<std::array<T, sz>> {
    outcome::result<void> encode(const std::array<T, sz> &array,
                                 common::Buffer &out) {
      return collection::encodeCollection(gsl::make_span(array), out);
    }
  };

  /// encodes common::Hash256
  template <>
  struct TypeEncoder<common::Hash256> {
    outcome::result<void> encode(const common::Hash256 &value,
                                 common::Buffer &out) {
      return collection::encodeCollection(gsl::make_span(value), out);
    }
  };

  /// encodes primitives::Invalid
  template <>
  struct TypeEncoder<primitives::Invalid> {
    outcome::result<void> encode(const primitives::Invalid &value,
                                 common::Buffer &out) {
      fixedwidth::encodeUInt8(value.error_, out);
      return outcome::success();
    }
  };

  /// encodes primitives::Unknown
  template <>
  struct TypeEncoder<primitives::Unknown> {
    outcome::result<void> encode(const primitives::Unknown &value,
                                 common::Buffer &out) {
      fixedwidth::encodeUInt8(value.error_, out);
      return outcome::success();
    }
  };

  /// encodes primitives::Valid
  template <>
  struct TypeEncoder<primitives::Valid> {
    outcome::result<void> encode(const primitives::Valid &value,
                                 common::Buffer &out) {
      fixedwidth::encodeUint64(value.priority_, out);
      OUTCOME_TRY(collection::encodeCollection(value.requires_, out));
      OUTCOME_TRY(collection::encodeCollection(value.provides_, out));
      fixedwidth::encodeUint64(value.longevity_, out);

      return outcome::success();
    }
  };

  /// encodes std::vector
  template <class T>
  struct TypeEncoder<std::vector<T>> {
    outcome::result<void> encode(const std::vector<T> &value,
                                 common::Buffer &out) {
      return collection::encodeCollection(value, out);
    }
  };

  /// decodes std::array
  template <class T, size_t sz>
  struct TypeDecoder<std::array<T, sz>> {
    using array_type = std::array<T, sz>;
    outcome::result<array_type> decode(common::ByteStream &stream) {
      OUTCOME_TRY(collection, collection::decodeCollection<T>(stream));
      if (collection.size() != sz) {
        return DecodeError::INVALID_DATA;
      }
      array_type array;
      std::copy(collection.begin(), collection.end(), array.begin());
      return array;
    }
  };

  /// decodes common::Hash256
  template <>
  struct TypeDecoder<common::Hash256> {
    outcome::result<common::Hash256> decode(common::ByteStream &stream) {
      OUTCOME_TRY(collection,
                  collection::decodeCollection<common::byte_t>(stream));
      common::Hash256 hash;
      if (collection.size() != common::Hash256::size()) {
        return DecodeError::INVALID_DATA;
      }
      std::copy(collection.begin(), collection.end(), hash.begin());
      return hash;
    }
  };

  /// decodes primitives::Invalid
  template <>
  struct TypeDecoder<primitives::Invalid> {
    outcome::result<primitives::Invalid> decode(common::ByteStream &stream) {
      OUTCOME_TRY(value, fixedwidth::decodeUint8(stream));
      return primitives::Invalid{value};
    }
  };

  /// decodes primitives::Unknown
  template <>
  struct TypeDecoder<primitives::Unknown> {
    outcome::result<primitives::Unknown> decode(common::ByteStream &stream) {
      OUTCOME_TRY(value, fixedwidth::decodeUint8(stream));
      return primitives::Unknown{value};
    }
  };

  /// decodes primitives::Valid
  template <>
  struct TypeDecoder<primitives::Valid> {
    outcome::result<primitives::Valid> decode(common::ByteStream &stream) {
      OUTCOME_TRY(priority, fixedwidth::decodeUint64(stream));
      OUTCOME_TRY(
          requires,
          collection::decodeCollection<primitives::TransactionTag>(stream));
      OUTCOME_TRY(
          provides,
          collection::decodeCollection<primitives::TransactionTag>(stream));
      OUTCOME_TRY(longevity, fixedwidth::decodeUint64(stream));

      return primitives::Valid{priority, requires, provides, longevity};
    }
  };

  /// decodes primitives::TransactionTag
  template <>
  struct TypeDecoder<primitives::TransactionTag> {
    outcome::result<primitives::TransactionTag> decode(
        common::ByteStream &stream) {
      return collection::decodeCollection<uint8_t>(stream);
    }
  };
}  // namespace kagome::scale

namespace kagome::primitives {
  using kagome::common::Buffer;
  using kagome::common::Hash256;
  using ArrayChar8Encoder = scale::TypeEncoder<std::array<uint8_t, 8>>;
  using ApiEncoder = scale::TypeEncoder<std::pair<ArrayChar8Encoder, uint32_t>>;
  using kagome::scale::collection::decodeCollection;
  using kagome::scale::collection::decodeString;
  using kagome::scale::collection::encodeBuffer;
  using kagome::scale::collection::encodeCollection;
  using kagome::scale::collection::encodeString;
  using kagome::scale::compact::decodeInteger;
  using kagome::scale::compact::encodeInteger;
  using kagome::scale::fixedwidth::decodeUint32;
  using kagome::scale::fixedwidth::decodeUint64;
  using kagome::scale::fixedwidth::encodeUint32;
  using kagome::scale::fixedwidth::encodeUint64;
  using kagome::scale::variant::decodeVariant;
  using kagome::scale::variant::encodeVariant;

  outcome::result<Buffer> ScaleCodecImpl::encodeBlock(
      const Block &block) const {
    Buffer out;
    OUTCOME_TRY(encoded_header, encodeBlockHeader(block.header()));
    out.putBuffer(encoded_header);

    // put number of extrinsics
    OUTCOME_TRY(encodeInteger(block.extrinsics().size(), out));

    for (auto &&extrinsic : block.extrinsics()) {
      OUTCOME_TRY(encoded_extrinsic, encodeExtrinsic(extrinsic));
      out.putBuffer(encoded_extrinsic);
    }

    return out;
  }

  outcome::result<Block> ScaleCodecImpl::decodeBlock(Stream &stream) const {
    OUTCOME_TRY(header, decodeBlockHeader(stream));
    // first decode number of items
    OUTCOME_TRY(integer, decodeInteger(stream));
    auto items_count = integer.convert_to<uint64_t>();

    std::vector<Extrinsic> collection;
    collection.reserve(items_count);

    // now decode collection of extrinsics
    for (uint64_t i = 0u; i < items_count; ++i) {
      OUTCOME_TRY(extrinsic, decodeExtrinsic(stream));
      collection.push_back(extrinsic);
    }

    return Block(header, collection);
  }

  outcome::result<Buffer> ScaleCodecImpl::encodeBlockHeader(
      const BlockHeader &block_header) const {
    Buffer out;

    scale::TypeEncoder<Hash256> encoder;

    OUTCOME_TRY(encoder.encode(block_header.parentHash(), out));
    encodeUint64(block_header.number(), out);
    OUTCOME_TRY(encoder.encode(block_header.stateRoot(), out));
    OUTCOME_TRY(encoder.encode(block_header.extrinsicsRoot(), out));
    OUTCOME_TRY(encodeBuffer(block_header.digest(), out));

    return out;
  }

  outcome::result<BlockHeader> ScaleCodecImpl::decodeBlockHeader(
      Stream &stream) const {
    scale::TypeDecoder<Hash256> decoder;

    OUTCOME_TRY(parent_hash, decoder.decode(stream));
    OUTCOME_TRY(number, decodeUint64(stream));
    OUTCOME_TRY(state_root, decoder.decode(stream));
    OUTCOME_TRY(extrinsics_root, decoder.decode(stream));
    OUTCOME_TRY(digest, decodeCollection<uint8_t>(stream));

    return BlockHeader(parent_hash, number, state_root,
                       extrinsics_root, Buffer{digest});
  }

  outcome::result<Buffer> ScaleCodecImpl::encodeExtrinsic(
      const Extrinsic &extrinsic) const {
    return extrinsic.data();
  }
  outcome::result<Extrinsic> ScaleCodecImpl::decodeExtrinsic(
      Stream &stream) const {
    Buffer out;

    OUTCOME_TRY(extrinsic, decodeCollection<uint8_t>(stream));
    // extrinsic is an encoded byte array, so when we decode it from stream
    // we obtain just byte array, and in order to keep its form
    // we need do write its size first
    OUTCOME_TRY(encodeInteger(extrinsic.size(), out));
    // and then bytes as well
    out.put(extrinsic);

    return Extrinsic(out);
  }

  outcome::result<Buffer> ScaleCodecImpl::encodeVersion(
      const Version &version) const {
    Buffer out;
    OUTCOME_TRY(encodeString(version.specName(), out));
    OUTCOME_TRY(encodeString(version.implName(), out));
    encodeUint32(version.authoringVersion(), out);
    encodeUint32(version.implVersion(), out);
    OUTCOME_TRY(encodeCollection<Api>(version.apis(), out));
    return out;
  }

  outcome::result<Version> ScaleCodecImpl::decodeVersion(
      ScaleCodec::Stream &stream) const {
    OUTCOME_TRY(spec_name, decodeString(stream));
    OUTCOME_TRY(impl_name, decodeString(stream));
    OUTCOME_TRY(authoring_version, decodeUint32(stream));
    OUTCOME_TRY(impl_version, decodeUint32(stream));
    OUTCOME_TRY(apis, decodeCollection<Api>(stream));

    return Version(spec_name, impl_name, authoring_version, impl_version, apis);
  }

  outcome::result<Buffer> ScaleCodecImpl::encodeBlockId(
      const BlockId &blockId) const {
    Buffer out;
    OUTCOME_TRY(encodeVariant(blockId, out));
    return out;
  }

  outcome::result<BlockId> ScaleCodecImpl::decodeBlockId(
      ScaleCodec::Stream &stream) const {
    return decodeVariant<common::Hash256, BlockNumber>(stream);
  }

  outcome::result<Buffer> ScaleCodecImpl::encodeTransactionValidity(
      const TransactionValidity &transactionValidity) const {
    Buffer out;
    OUTCOME_TRY(encodeVariant(transactionValidity, out));
    return out;
  }

  outcome::result<TransactionValidity>
  ScaleCodecImpl::decodeTransactionValidity(ScaleCodec::Stream &stream) const {
    return decodeVariant<Invalid, Valid, Unknown>(stream);
  }
}  // namespace kagome::primitives

