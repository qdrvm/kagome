/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_SCALE_TYPES_HPP
#define KAGOME_SCALE_TYPES_HPP

#include <vector>

#include <boost/multiprecision/cpp_int.hpp>
#include <outcome/outcome.hpp>

namespace kagome::scale {
  /**
   * @brief convenience alias for arrays of bytes
   */
  using ByteArray = std::vector<uint8_t>;
  /**
   * @brief represents compact integer value
   */
  using CompactInteger = boost::multiprecision::cpp_int;

  /**
   * @brief OptionalBool is internal extended bool type
   */
  enum class OptionalBool : uint8_t { NONE = 0u, TRUE = 1u, FALSE = 2u };

  /**
   * Wrapper to encode a collection not as a common SCALE-encoded collection,
   * but just element-wise. Can only be encoded.
   */
  template <typename Collection>
  struct RawCollection {
    std::decay_t<Collection> collection;
  };

  template <class Stream,
            typename Collection,
            typename = std::enable_if_t<Stream::is_encoder_stream>>
  Stream &operator<<(Stream &s, const RawCollection<Collection> &v) {
    for (auto &i : v.collection) {
      s << i;
    }
    return s;
  }

}  // namespace kagome::scale

namespace kagome::scale::compact {
  /**
   * @brief categories of compact encoding
   */
  struct EncodingCategoryLimits {
    // min integer encoded by 2 bytes
    constexpr static size_t kMinUint16 = (1ul << 6u);
    // min integer encoded by 4 bytes
    constexpr static size_t kMinUint32 = (1ul << 14u);
    // min integer encoded as multibyte
    constexpr static size_t kMinBigInteger = (1ul << 30u);
  };
}  // namespace kagome::scale::compact
#endif  // KAGOME_SCALE_TYPES_HPP
