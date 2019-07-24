/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_JUSTIFICATION_HPP
#define KAGOME_JUSTIFICATION_HPP

namespace kagome::primitives {
  /**
   * Justification of the finalized block
   */
  struct Justification {};

  /**
   * @brief outputs object of type Justification to stream
   * @tparam Stream output stream type
   * @param s stream reference
   * @param v value to output
   * @return reference to stream
   */
  template <class Stream,
            typename = std::enable_if_t<Stream::is_encoder_stream>>
  Stream &operator<<(Stream &s, const Justification &v) {
    return s;
  }

  /**
   * @brief decodes object of type Justification from stream
   * @tparam Stream input stream type
   * @param s stream reference
   * @param v value to output
   * @return reference to stream
   */
  template <class Stream,
            typename = std::enable_if_t<Stream::is_decoder_stream>>
  Stream &operator>>(Stream &s, Justification &v) {
    return s;
  }
}  // namespace kagome::primitives

#endif  // KAGOME_JUSTIFICATION_HPP
