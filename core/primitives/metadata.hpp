/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_PRIMITIVES_OPAQUE_METADATA_HPP
#define KAGOME_CORE_PRIMITIVES_OPAQUE_METADATA_HPP

#include <cstdint>
#include <vector>

#include <scale/detail/fixed_width_integer.hpp>

#include "scale/tie.hpp"

namespace kagome::primitives {
  /**
   * Polkadot primitive, which is opaque representation of RuntimeMetadata
   */
  using OpaqueMetadata = std::vector<uint8_t>;

  using TypeId = scale::CompactInteger;

  struct Field {
    SCALE_TIE(4)

    std::optional<std::string> name;
    TypeId type_id;
    std::optional<std::string> type_name;
    std::vector<std::string> documentation;
  };

  struct CompositeType {
    SCALE_TIE(1)

    std::vector<Field> fields;
  };

  struct VariantAlternative {
    SCALE_TIE(4)

    std::string name;
    std::vector<Field> fields;
    uint8_t index;
    std::vector<std::string> documentation;
  };

  struct VariantType {
    SCALE_TIE(1)

    std::vector<VariantAlternative> alternatives;
  };

  struct SequenceType {
    SCALE_TIE(2)

    uint32_t length;
    TypeId type_id;
  };

  struct TupleType {
    SCALE_TIE(1)

    std::vector<TypeId> members;
  };

  template <typename T>
  struct FixedIntegerWrapper {
    T integer;
  };

  template <typename Stream,
            typename T,
            typename = std::enable_if_t<Stream::is_decoder_stream>>
  Stream &operator>>(Stream &stream, FixedIntegerWrapper<T> &wrapper) {
    scale::CompactInteger i;
    stream >> i;
    wrapper.integer = i.convert_to<T>();
    return stream;
  }

  using PrimitiveType =
      boost::variant<bool,
                     char,
                     std::string,
                     uint8_t,
                     uint16_t,
                     uint32_t,
                     uint64_t,
                     FixedIntegerWrapper<boost::multiprecision::uint128_t>,
                     FixedIntegerWrapper<boost::multiprecision::uint256_t>,
                     int8_t,
                     int16_t,
                     int32_t,
                     int64_t,
                     FixedIntegerWrapper<boost::multiprecision::int128_t>,
                     FixedIntegerWrapper<boost::multiprecision::int256_t>>;

  struct BitSequence {
    SCALE_TIE(2)

    // https://docs.rs/bitvec/latest/bitvec/store/trait.BitStore.html
    TypeId store_order;
    // https://docs.rs/bitvec/latest/bitvec/order/trait.BitOrder.html
    TypeId order_type;
  };

  struct VaryingLengthSequence {
    SCALE_TIE(1)

    TypeId type_id;
  };

  using TypeVariant = boost::variant<CompositeType,
                                     VariantType,
                                     VaryingLengthSequence,
                                     SequenceType,
                                     TupleType,
                                     PrimitiveType,
                                     TypeId,
                                     BitSequence>;

  struct RegistryTypeEntry {
    SCALE_TIE(5)

    TypeId id;
    std::vector<std::string> type_path;

    struct GenericParameter {
      SCALE_TIE(2)
      std::string name;
      std::optional<TypeId> type_id;
    };

    std::vector<GenericParameter> generic_parameters;
    TypeVariant definition;
    std::vector<std::string> documentation;
  };

  enum class StorageHasher {
    Blake2_128,
    Blake2_256,
    Blake2_128_Concat,
    XX_128,
    XX_256,
    XX_64_Concat,
    Identity
  };

  struct StorageMap {
    SCALE_TIE(3)

    std::vector<StorageHasher> hasher;
    TypeId key;
    TypeId value;
  };

  struct StorageEntry {
    SCALE_TIE(5)

    std::string name;
    enum class StorageModifier {
      // returns an optional indicating if value is present
      Optional,
      // returns the default value if the entry is not present
      Default
    } modifier;
    boost::variant<TypeId> value_type;
    common::Buffer default_value;
    std::vector<std::string> documentation;
  };

  struct StorageMetadata {
    SCALE_TIE(2)

    common::Buffer prefix;
    std::vector<StorageEntry> entries;
  };

  struct ConstantMetadata {
    SCALE_TIE(4)

    std::string name;
    TypeId type;
    common::Buffer value;
    std::vector<std::string> documentation;
  };

  struct PalletMetadata {
    SCALE_TIE(7)

    std::string pallet_name;
    std::optional<StorageMetadata> storage_metadata;
    std::optional<TypeId> pallet_calls;
    std::optional<TypeId> pallet_events;
    std::vector<ConstantMetadata> constant_metadata;
    std::optional<TypeId> pallet_error;
    common::Blob<8UL> pallet_index;
  };

  struct ExtrinsicMetadata {
    SCALE_TIE(3)

    std::string signed_extension_id;
    TypeId signed_extension_type;
    TypeId additional_data_type;
  };

  struct Metadata {
    SCALE_TIE(7)

    Metadata() = default;

    explicit Metadata(const OpaqueMetadata &opaque) {
      BOOST_ASSERT(std::equal(magic_number.begin(),
                              magic_number.end(),
                              gsl::make_span(opaque).subspan(0, 4).begin()));
      auto decoded =
          scale::decode<Metadata>(gsl::make_span(opaque).subspan(4)).value();
      *this = decoded;
    }

    static constexpr std::string_view magic_number = "meta";
    uint8_t metadata_version;
    std::vector<RegistryTypeEntry> type_registry;
    std::vector<PalletMetadata> pallets_metadata;
    TypeId extrinsic_type_id;
    uint8_t extrinsic_format_version;
    std::vector<ExtrinsicMetadata> extrinsics_metadata;
    TypeId runtime_type_id;
  };
}  // namespace kagome::primitives

#endif  // KAGOME_CORE_PRIMITIVES_OPAQUE_METADATA_HPP
