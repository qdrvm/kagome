/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_SCALE_TYPE_DECODER_HPP
#define KAGOME_SCALE_TYPE_DECODER_HPP

#include "common/scale/compact.hpp"
#include "common/scale/fixedwidth.hpp"
#include "common/scale/boolean.hpp"
#include "common/scale/types.hpp"
#include "common/scale/util.hpp"

namespace kagome::common::scale {
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
    TypeDecodeResult<T> decode(Stream &stream) {
      static_assert(std::is_integral<T>(),
                    "Only integral types are supported. You need to define "
                    "your own TypeDecoder specialization for custom type.");
      auto optional = impl::decodeInteger<T>(stream);
      if (!optional.has_value()) {
        return expected::Error{DecodeError::kNotEnoughData};
      }

      return expected::Value{*optional};
    }
  };

  /**
   * @class TypeDecoder<bool> is specialization of TypeDecoder class
   * it implements decoding bool values from stream
   */
  template <>
  struct TypeDecoder<bool> {
    auto decode(Stream &stream) {
      return boolean::decodeBool(stream);
    }
  };

  /**
   * @class TypeDecoder<tribool> is specialization of TypeDecoder class
   * it implements decoding tribool values from stream
   */
  template <>
  struct TypeDecoder<tribool> {
    auto decode(Stream &stream) {
      return boolean::decodeTribool(stream);
    }
  };
}  // namespace kagome::common::scale

#endif  // KAGOME_SCALE_TYPE_DECODER_HPP
