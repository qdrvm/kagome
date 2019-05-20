/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "primitives/impl/scale_codec_impl.hpp"

#include "primitives/block.hpp"
#include "primitives/block_id.hpp"
#include "primitives/parachain_host.hpp"
#include "primitives/transaction_validity.hpp"
#include "primitives/version.hpp"
#include "scale/blob_codec.hpp"
#include "scale/buffer_codec.hpp"
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
      for (size_t i = 0; i < sz; i++) {
        fixedwidth::encodeUInt8(gsl::at(array, i), out);
      }
      return outcome::success();
    }
  };

  /// encodes common::Hash256
  template <>
  struct TypeEncoder<common::Hash256> {
    outcome::result<void> encode(const common::Hash256 &value,
                                 common::Buffer &out) {
      for (size_t i = 0; i < common::Hash256::size(); i++) {
        fixedwidth::encodeUInt8(value[i], out);
      }
      return outcome::success();
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

  /// encodes primitives::Extrinsic
  template <>
  struct TypeEncoder<primitives::Extrinsic> {
    outcome::result<void> encode(const primitives::Extrinsic &value,
                                 common::Buffer &out) {
      out.putBuffer(value.data());
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

  /// encodes parachain::Chain
  template <>
  struct TypeEncoder<primitives::parachain::Chain> {
    outcome::result<void> encode(const primitives::parachain::Chain &value,
                                 common::Buffer &out) {
      return variant::encodeVariant(value, out);
    }
  };

  /// encodes object which has no content
  template <class T>
  struct EmptyEncoder {
    outcome::result<void> encode(const T &value, common::Buffer &out) {
      return outcome::success();  // no content
    }
  };

  /// encodes object of class primitives::parachain::Relay
  template <>
  struct TypeEncoder<primitives::parachain::Relay>
      : public EmptyEncoder<primitives::parachain::Relay> {};

  // Decode part

  /// decodes std::array
  template <class T, size_t sz>
  struct TypeDecoder<std::array<T, sz>> {
    using array_type = std::array<T, sz>;
    outcome::result<array_type> decode(common::ByteStream &stream) {
      array_type array;

      for (size_t i = 0; i < sz; i++) {
        OUTCOME_TRY(byte, fixedwidth::decodeUint8(stream));
        array[i] = byte;
      }
      return array;
    }
  };

  /// decodes common::Hash256
  template <>
  struct TypeDecoder<common::Hash256> {
    outcome::result<common::Hash256> decode(common::ByteStream &stream) {
      common::Hash256 hash;
      for (size_t i = 0; i < common::Hash256::size(); i++) {
        OUTCOME_TRY(byte, fixedwidth::decodeUint8(stream));
        hash[i] = byte;
      }
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

  /// decodes primitives::Extrinsic
  template <>
  struct TypeDecoder<primitives::Extrinsic> {
    outcome::result<primitives::Extrinsic> decode(common::ByteStream &stream) {
      common::Buffer out;

      OUTCOME_TRY(extrinsic, collection::decodeCollection<uint8_t>(stream));
      // extrinsic is an encoded byte array, so when we decode it from stream
      // we obtain just byte array, and in order to keep its form
      // we need do write its size first
      OUTCOME_TRY(compact::encodeInteger(extrinsic.size(), out));
      // and then bytes as well
      out.put(extrinsic);

      return primitives::Extrinsic(out);
    }
  };

  /// decodes primitives::parachain::Chain
  template <>
  struct TypeDecoder<primitives::parachain::Chain> {
    outcome::result<primitives::parachain::Chain> decode(
        common::ByteStream &stream) {
      return variant::decodeVariant<primitives::parachain::Relay,
                                    primitives::parachain::Parachain>(stream);
    }
  };

  /// decodes object of empty class
  template <class T>
  struct EmptyDecoder {
    outcome::result<T> decode(common::ByteStream &) {
      return T{};
    }
  };

  /// decodes fake type primitives::parachain::Relay, which has no content
  template <>
  struct TypeDecoder<primitives::parachain::Relay>
      : public EmptyDecoder<primitives::parachain::Relay> {};

}  // namespace kagome::scale

namespace kagome::primitives {
  using kagome::common::Buffer;
  using kagome::common::Hash256;
  using ArrayChar8Encoder = scale::TypeEncoder<std::array<uint8_t, 8>>;
  using ApiEncoder = scale::TypeEncoder<std::pair<ArrayChar8Encoder, uint32_t>>;
  using kagome::scale::collection::decodeCollection;
  using kagome::scale::collection::decodeString;
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
  using primitives::InherentData;

  ScaleCodecImpl::ScaleCodecImpl() {
    buffer_codec_ = std::make_unique<scale::BufferScaleCodec>();
  }

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
    OUTCOME_TRY(digest, buffer_codec_->encode(block_header.digest()));
    // TODO(yuraz): PRE-119 refactor ScaleEncode interface
    out.putBuffer(digest);

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

    return BlockHeader(std::move(parent_hash), number, std::move(state_root),
                       std::move(extrinsics_root), Buffer{std::move(digest)});
  }

  outcome::result<Buffer> ScaleCodecImpl::encodeExtrinsic(
      const Extrinsic &extrinsic) const {
    Buffer out;
    OUTCOME_TRY(scale::TypeEncoder<Extrinsic>{}.encode(extrinsic, out));
    return out;
  }
  outcome::result<Extrinsic> ScaleCodecImpl::decodeExtrinsic(
      Stream &stream) const {
    return scale::TypeDecoder<Extrinsic>{}.decode(stream);
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

  outcome::result<Buffer> ScaleCodecImpl::encodeInherentData(
      const InherentData &inherentData) const {
    auto &data = inherentData.getDataCollection();

    // TODO(Harrm) PRE-153 Change to vectors of ref wrappers to avoid copying
    // vectors
    std::vector<InherentData::InherentIdentifier> ids;
    ids.reserve(data.size());
    std::vector<std::vector<uint8_t>> vals;
    vals.reserve(data.size());

    for (auto &pair : data) {
      ids.push_back(pair.first);
      vals.push_back(pair.second.toVector());
    }

    Buffer res;
    OUTCOME_TRY(encodeCollection(ids, res));
    OUTCOME_TRY(encodeCollection(vals, res));
    return res;
  }

  outcome::result<InherentData> ScaleCodecImpl::decodeInherentData(
      Stream &stream) const {
    OUTCOME_TRY(ids,
                decodeCollection<InherentData::InherentIdentifier>(stream));
    OUTCOME_TRY(vals, decodeCollection<std::vector<uint8_t>>(stream));

    InherentData data;
    for (size_t i = 0; i < ids.size(); i++) {
      OUTCOME_TRY(data.putData(ids[i], Buffer{vals[i]}));
    }

    return data;
  }

  outcome::result<Buffer> ScaleCodecImpl::encodeAuthorityIds(
      const std::vector<kagome::primitives::AuthorityId> &ids) const {
    Buffer out;
    OUTCOME_TRY(encodeCollection<AuthorityId>(ids, out));
    return out;
  }

  outcome::result<std::vector<AuthorityId>> ScaleCodecImpl::decodeAuthorityIds(
      kagome::primitives::ScaleCodec::Stream &stream) const {
    return decodeCollection<AuthorityId>(stream);
  }

  outcome::result<Buffer> ScaleCodecImpl::encodeDutyRoster(
      const parachain::DutyRoster &duty_roster) const {
    Buffer out;
    OUTCOME_TRY(encodeCollection<parachain::Chain>(duty_roster, out));
    return out;
  }

  outcome::result<parachain::DutyRoster> ScaleCodecImpl::decodeDutyRoster(
      ScaleCodec::Stream &stream) const {
    return decodeCollection<parachain::Chain>(stream);
  }
}  // namespace kagome::primitives
