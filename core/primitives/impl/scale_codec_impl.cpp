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

#include "scale/scale.hpp"

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
      OUTCOME_TRY(data, collection::decodeCollection<uint8_t>(stream));
      return primitives::Extrinsic{common::Buffer(std::move(data))};
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
  using kagome::scale::collection::decodeCollection;
  using kagome::scale::collection::decodeString;
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
    OUTCOME_TRY(data, scale::encode(block));
    return Buffer(data);
  }

  outcome::result<Block> ScaleCodecImpl::decodeBlock(Stream &stream) const {
    OUTCOME_TRY(header, decodeBlockHeader(stream));
    OUTCOME_TRY(extrinsics, decodeCollection<Extrinsic>(stream));

    return Block{header, extrinsics};
  }

  outcome::result<Buffer> ScaleCodecImpl::encodeBlockHeader(
      const BlockHeader &block_header) const {
    OUTCOME_TRY(data, scale::encode(block_header));
    return Buffer(data);
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
    OUTCOME_TRY(data, scale::encode(extrinsic));
    return Buffer(data);
  }
  outcome::result<Extrinsic> ScaleCodecImpl::decodeExtrinsic(
      Stream &stream) const {
    return scale::TypeDecoder<Extrinsic>{}.decode(stream);
  }

  outcome::result<Buffer> ScaleCodecImpl::encodeVersion(
      const Version &version) const {
    OUTCOME_TRY(data, scale::encode(version));
    return Buffer(data);
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
      const BlockId &block_id) const {
    OUTCOME_TRY(data, scale::encode(block_id));
    return Buffer(data);
  }

  outcome::result<BlockId> ScaleCodecImpl::decodeBlockId(
      ScaleCodec::Stream &stream) const {
    return decodeVariant<common::Hash256, BlockNumber>(stream);
  }

  outcome::result<Buffer> ScaleCodecImpl::encodeTransactionValidity(
      const TransactionValidity &v) const {
    OUTCOME_TRY(data, scale::encode(v));
    return Buffer(data);
  }

  outcome::result<TransactionValidity>
  ScaleCodecImpl::decodeTransactionValidity(ScaleCodec::Stream &stream) const {
    return decodeVariant<Invalid, Valid, Unknown>(stream);
  }

  outcome::result<Buffer> ScaleCodecImpl::encodeInherentData(
      const InherentData &v) const {
    OUTCOME_TRY(data, scale::encode(v));
    return Buffer(data);
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
    OUTCOME_TRY(data, scale::encode(ids));
    return Buffer(data);
  }

  outcome::result<std::vector<AuthorityId>> ScaleCodecImpl::decodeAuthorityIds(
      kagome::primitives::ScaleCodec::Stream &stream) const {
    return decodeCollection<AuthorityId>(stream);
  }

  outcome::result<Buffer> ScaleCodecImpl::encodeDutyRoster(
      const parachain::DutyRoster &duty_roster) const {
    OUTCOME_TRY(data, scale::encode(duty_roster));
    return Buffer(data);
  }

  outcome::result<parachain::DutyRoster> ScaleCodecImpl::decodeDutyRoster(
      ScaleCodec::Stream &stream) const {
    return decodeCollection<parachain::Chain>(stream);
  }

  outcome::result<Buffer> ScaleCodecImpl::encodeDigest(
      const Digest &digest) const {
    OUTCOME_TRY(data, scale::encode(digest));
    return Buffer(data);
  }

  outcome::result<Digest> ScaleCodecImpl::decodeDigest(
      ScaleCodec::Stream &stream) const {
    return buffer_codec_->decode(stream);
  }

  outcome::result<Buffer> ScaleCodecImpl::encodeScheduledChange(
      const ScheduledChange &value) const {
    OUTCOME_TRY(data, scale::encode(value));
    return Buffer(data);
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
