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
#include "scale/optional.hpp"
#include "scale/scale_error.hpp"
#include "scale/variant.hpp"

#include "scale/scale_encoder_stream.hpp"

// TODO(yuraz): replace all of this by operators in corresponding files

/// custom types encoders and decoders
namespace kagome::scale {
  // Decode part

  /// decodes std::array
  template <class T, size_t sz>
  struct TypeDecoder<std::array<T, sz>> {
    using array_type = std::array<T, sz>;
    outcome::result<array_type> decode(common::ByteStream &stream) {
      array_type array;

      for (size_t i = 0u; i < sz; i++) {
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
      ScaleEncoderStream s;
      OUTCOME_TRY(compact::encodeInteger(extrinsic.size(), s));
      out += s.getBuffer();
      // and then bytes as well
      out.put(extrinsic);

      return primitives::Extrinsic{out};
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
    outcome::result<T> decode(common::ByteStream & /*unused*/) {  // NOLINT
      return T{};
    }
  };

  /// decodes fake type primitives::parachain::Relay, which has no content
  template <>
  struct TypeDecoder<primitives::parachain::Relay>
      : public EmptyDecoder<primitives::parachain::Relay> {};

  template <>
  struct TypeDecoder<primitives::ScheduledChange> {
    outcome::result<primitives::ScheduledChange> decode(
        common::ByteStream &stream) {
      OUTCOME_TRY(next_authorities,
                  collection::decodeCollection<
                      std::pair<primitives::AuthorityId, uint64_t>>(stream));
      OUTCOME_TRY(delay, fixedwidth::decodeUint64(stream));

      return primitives::ScheduledChange{std::move(next_authorities), delay};
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
  using kagome::scale::ScaleEncoderStream;
  using kagome::scale::compact::decodeInteger;
  using kagome::scale::compact::encodeInteger;
  using kagome::scale::fixedwidth::decodeUint32;
  using kagome::scale::fixedwidth::decodeUint64;
  using kagome::scale::optional::decodeOptional;
  using kagome::scale::variant::decodeVariant;
  using kagome::scale::variant::encodeVariant;
  using primitives::InherentData;
  ScaleCodecImpl::ScaleCodecImpl() {
    buffer_codec_ = std::make_unique<scale::BufferScaleCodec>();
  }

  outcome::result<Buffer> ScaleCodecImpl::encodeBlock(
      const Block &block) const {
    Buffer out;
    OUTCOME_TRY(encoded_header, encodeBlockHeader(block.header));
    out.putBuffer(encoded_header);

    ScaleEncoderStream s;

    s << block.extrinsics;

    return out += s.getBuffer();
  }

  outcome::result<Block> ScaleCodecImpl::decodeBlock(Stream &stream) const {
    OUTCOME_TRY(header, decodeBlockHeader(stream));
    OUTCOME_TRY(extrinsics, decodeCollection<Extrinsic>(stream));

    return Block{header, extrinsics};
  }

  outcome::result<Buffer> ScaleCodecImpl::encodeBlockHeader(
      const BlockHeader &block_header) const {
    ScaleEncoderStream s;
    s << block_header;
    return s.getBuffer();
  }

  outcome::result<BlockHeader> ScaleCodecImpl::decodeBlockHeader(
      Stream &stream) const {
    scale::TypeDecoder<Hash256> decoder;
    OUTCOME_TRY(parent_hash, decoder.decode(stream));
    OUTCOME_TRY(number, decodeUint64(stream));
    OUTCOME_TRY(state_root, decoder.decode(stream));
    OUTCOME_TRY(extrinsics_root, decoder.decode(stream));
    OUTCOME_TRY(digest, decodeCollection<uint8_t>(stream));

    return BlockHeader{parent_hash, number, state_root, extrinsics_root,
                       Buffer{std::move(digest)}};
  }

  outcome::result<Buffer> ScaleCodecImpl::encodeExtrinsic(
      const Extrinsic &extrinsic) const {
    ScaleEncoderStream s;
    try {
      s << extrinsic;
    } catch (...) {
      std::terminate();
    }
    return s.getBuffer();
  }
  outcome::result<Extrinsic> ScaleCodecImpl::decodeExtrinsic(
      Stream &stream) const {
    return scale::TypeDecoder<Extrinsic>{}.decode(stream);
  }

  outcome::result<Buffer> ScaleCodecImpl::encodeVersion(
      const Version &version) const {
    ScaleEncoderStream s;
    s << version;
    return s.getBuffer();
  }

  outcome::result<Version> ScaleCodecImpl::decodeVersion(
      ScaleCodec::Stream &stream) const {
    OUTCOME_TRY(spec_name, decodeString(stream));
    OUTCOME_TRY(impl_name, decodeString(stream));
    OUTCOME_TRY(authoring_version, decodeUint32(stream));
    OUTCOME_TRY(impl_version, decodeUint32(stream));
    OUTCOME_TRY(apis, decodeCollection<Api>(stream));

    return Version{spec_name, impl_name, authoring_version, impl_version, apis};
  }

  outcome::result<Buffer> ScaleCodecImpl::encodeBlockId(
      const BlockId &blockId) const {
    ScaleEncoderStream s;
    s << blockId;
    return s.getBuffer();
  }

  outcome::result<BlockId> ScaleCodecImpl::decodeBlockId(
      ScaleCodec::Stream &stream) const {
    return decodeVariant<common::Hash256, BlockNumber>(stream);
  }

  outcome::result<Buffer> ScaleCodecImpl::encodeTransactionValidity(
      const TransactionValidity &transactionValidity) const {
    ScaleEncoderStream s;
    s << transactionValidity;
    return s.getBuffer();
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
    std::vector<InherentIdentifier> ids;
    ids.reserve(data.size());
    std::vector<std::vector<uint8_t>> vals;
    vals.reserve(data.size());

    for (auto &pair : data) {
      ids.push_back(pair.first);
      vals.push_back(pair.second.toVector());
    }

    ScaleEncoderStream s;
    s << ids << vals;
    return s.getBuffer();
  }

  outcome::result<InherentData> ScaleCodecImpl::decodeInherentData(
      Stream &stream) const {
    OUTCOME_TRY(ids, decodeCollection<InherentIdentifier>(stream));
    OUTCOME_TRY(vals, decodeCollection<std::vector<uint8_t>>(stream));

    InherentData data;
    for (size_t i = 0; i < ids.size(); i++) {
      OUTCOME_TRY(data.putData(ids[i], Buffer{vals[i]}));
    }

    return data;
  }

  outcome::result<Buffer> ScaleCodecImpl::encodeAuthorityIds(
      const std::vector<kagome::primitives::AuthorityId> &ids) const {
    ScaleEncoderStream s;
    s << ids;
    return s.getBuffer();
  }

  outcome::result<std::vector<AuthorityId>> ScaleCodecImpl::decodeAuthorityIds(
      kagome::primitives::ScaleCodec::Stream &stream) const {
    return decodeCollection<AuthorityId>(stream);
  }

  outcome::result<Buffer> ScaleCodecImpl::encodeDutyRoster(
      const parachain::DutyRoster &duty_roster) const {
    ScaleEncoderStream s;
    s << duty_roster;
    return s.getBuffer();
  }

  outcome::result<parachain::DutyRoster> ScaleCodecImpl::decodeDutyRoster(
      ScaleCodec::Stream &stream) const {
    return decodeCollection<parachain::Chain>(stream);
  }

  outcome::result<Buffer> ScaleCodecImpl::encodeDigest(
      const Digest &digest) const {
    return buffer_codec_->encode(digest);
  }

  outcome::result<Digest> ScaleCodecImpl::decodeDigest(
      ScaleCodec::Stream &stream) const {
    return buffer_codec_->decode(stream);
  }

  outcome::result<Buffer> ScaleCodecImpl::encodeScheduledChange(
      const ScheduledChange &value) const {
    ScaleEncoderStream s;
    try {
      s << value;
    } catch (...) {
      std::terminate();
    }

    return s.getBuffer();
  }

  outcome::result<std::optional<ScheduledChange>>
  ScaleCodecImpl::decodeScheduledChange(ScaleCodec::Stream &stream) const {
    return decodeOptional<ScheduledChange>(stream);
  }

  outcome::result<std::optional<primitives::ForcedChange>>
  ScaleCodecImpl::decodeForcedChange(ScaleCodec::Stream &stream) const {
    return scale::optional::decodeOptional<
        std::pair<BlockNumber, ScheduledChange>>(stream);
  }

  outcome::result<std::vector<primitives::WeightedAuthority>>
  ScaleCodecImpl::decodeGrandpaAuthorities(ScaleCodec::Stream &stream) const {
    return scale::collection::decodeCollection<primitives::WeightedAuthority>(
        stream);
  }
}  // namespace kagome::primitives
