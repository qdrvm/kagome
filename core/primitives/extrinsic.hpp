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

#ifndef KAGOME_PRIMITIVES_EXTRINSIC_HPP
#define KAGOME_PRIMITIVES_EXTRINSIC_HPP

#include "common/buffer.hpp"

namespace kagome::primitives {
  /**
   * @brief Extrinsic class represents extrinsic
   */
  struct Extrinsic {
    common::Buffer data;  ///< extrinsic content as byte array

    inline bool operator==(const Extrinsic &rhs) const {
      return data == rhs.data;
    }
  };

  /**
   * @brief outputs object of type Extrinisic to stream
   * @tparam Stream output stream type
   * @param s stream reference
   * @param v value to output
   * @return reference to stream
   */
  template <class Stream,
            typename = std::enable_if_t<Stream::is_encoder_stream>>
  Stream &operator<<(Stream &s, const Extrinsic &v) {
    return s << v.data;
  }

  /**
   * @brief decodes object of type Extrinisic from stream
   * @tparam Stream input stream type
   * @param s stream reference
   * @param v value to output
   * @return reference to stream
   */
  template <class Stream,
            typename = std::enable_if_t<Stream::is_decoder_stream>>
  Stream &operator>>(Stream &s, Extrinsic &v) {
    return s >> v.data;
  }
}  // namespace kagome::primitives

#endif  // KAGOME_PRIMITIVES_EXTRINSIC_HPP
