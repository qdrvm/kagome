/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_SCALE_HPP
#define KAGOME_SCALE_HPP

#include <vector>

#include <boost/system/system_error.hpp>
#include <boost/throw_exception.hpp>
#include <gsl/span>
#include <outcome/outcome.hpp>
#include "scale/scale_decoder_stream.hpp"
#include "scale/scale_encoder_stream.hpp"

namespace kagome::scale {
  /**
   * @brief convenience function for encoding primitives data to stream
   * @tparam T primitive type
   * @param t data to encode
   * @return encoded data
   */
  template <class T>
  outcome::result<std::vector<uint8_t>> encode(T &&t) {
    try {
      ScaleEncoderStream s;
      s << t;
      return s.data();
    } catch (const boost::system::system_error &e) {
      return e.code();
    }
  }

  /**
   * @brief convenience function for decoding primitives data from stream
   * @param span
   * @return
   */
  template <class T>
  outcome::result<T> decode(gsl::span<const uint8_t> span) {
    try {
      T t{};
      ScaleDecoderStream s(span);
      s >> t;
      return t;
    } catch (const boost::system::system_error &e) {
      return e.code();
    }
  }

  template <class T>
  outcome::result<T> decode(ScaleDecoderStream &s) {
    try {
      T t{};
      s >> t;
      return t;
    } catch (const boost::system::system_error &e) {
      return e.code();
    }
  }
}  // namespace kagome::scale
#endif  // KAGOME_SCALE_HPP
