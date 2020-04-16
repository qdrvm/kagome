/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 *
 * Kagome is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * Kagome is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
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
   * @tparam Args primitive types to be encoded
   * @param args data to encode
   * @return encoded data
   */
  template <typename... Args>
  outcome::result<std::vector<uint8_t>> encode(Args &&... args) {
    ScaleEncoderStream s {};
    try {
      (s << ... << std::forward<Args>(args));
    } catch (std::system_error &e) {
      return outcome::failure(e.code());
    }
    return s.data();
  }

  /**
   * @brief convenience function for decoding primitives data from stream
   * @tparam T primitive type that is decoded from provided span
   * @param span of bytes with encoded data
   * @return decoded T
   */
  template <class T>
  outcome::result<T> decode(gsl::span<const uint8_t> span) {
    T t{};
    ScaleDecoderStream s(span);
    try {
      s >> t;
    } catch (std::system_error &e) {
      return outcome::failure(e.code());
    }

    return outcome::success(t);
  }
}  // namespace kagome::scale
#endif  // KAGOME_SCALE_HPP
