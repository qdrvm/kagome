/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_SCALE_TYPE_DECODER_HPP
#define KAGOME_SCALE_TYPE_DECODER_HPP

#include "scale/boolean.hpp"
#include "scale/compact.hpp"
#include "scale/fixedwidth.hpp"
#include "scale/types.hpp"
#include "scale/util.hpp"

// TODO(yuraz): PRE-119 conception of TypeEncoder/TypeDecoder needs to be
// refactored

namespace kagome::scale {
  /**
   * Type decoders are nested decoders used to decode types in optionals,
   * variants, collections and tuples.
   * If you need to decode custom type, you need to implement specialisation of
   * TypeDecoder class.
   * It is obligued to have one function:
   *
   *            TypeDecoderResult<T> decode(Stream & stream);
   *
   * Where Stream is the Stream interface defined in types.hpp.
   * It is assumed that decoder takes data from input stream,
   *
   * Type decoders for basic integral types and bool and tribool are
   * provided from the box.
   */

  /**
   * @class TypeDecoder implements decoding integral types from stream
   * @tparam T any integral type
   */
  template <class T>
  struct TypeDecoder {
    outcome::result<T> decode(common::ByteStream &stream) {
      static_assert(std::is_integral<T>(),
                    "Only integral types are supported. You need to define "
                    "your own TypeDecoder specialization for custom type.");
      return impl::decodeInteger<T>(stream);
    }
  };

  /**
   * @class TypeDecoder<bool> is specialization of TypeDecoder class
   * it implements decoding bool values from stream
   */
  template <>
  struct TypeDecoder<bool> {
    auto decode(common::ByteStream &stream) {
      return boolean::decodeBool(stream);
    }
  };

  /**
   * @class TypeDecoder<tribool> is specialization of TypeDecoder class
   * it implements decoding tribool values from stream
   */
  template <>
  struct TypeDecoder<tribool> {
    auto decode(common::ByteStream &stream) {
      return boolean::decodeTribool(stream);
    }
  };

  /**
   * @class TypeDecoder<std::pair<F,S>> is specialization of TypeDecoder class
   * it implements decoding std::pair from stream
   * @tparam F first type
   * @tparam S second type
   */
  template <class F, class S>
  struct TypeDecoder<std::pair<F, S>> {
    using value_type = std::pair<F, S>;
    outcome::result<value_type> decode(common::ByteStream &stream) {
      OUTCOME_TRY(first_value, TypeDecoder<F>{}.decode(stream));
      OUTCOME_TRY(second_value, TypeDecoder<S>{}.decode(stream));
      return std::pair<F, S>(first_value, second_value);
    }
  };
}  // namespace kagome::scale

#endif  // KAGOME_SCALE_TYPE_DECODER_HPP
