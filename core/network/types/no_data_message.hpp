/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_NETWORK_TYPES_NO_DATA_HPP
#define KAGOME_CORE_NETWORK_TYPES_NO_DATA_HPP

#include <algorithm>
#include <vector>

#include "primitives/common.hpp"

namespace kagome::network {

  /**
   * Is the structure without any data.
   */
  struct NoData {};

  /**
   * @brief compares two Status instances
   * @param lhs first instance
   * @param rhs second instance
   * @return true if equal false otherwise
   */
  inline bool operator==(const NoData &lhs, const NoData &rhs) {
    return true;
  }

  /**
   * @brief outputs object of type Status to stream
   * @tparam Stream output stream type
   * @param s stream reference
   * @param v value to output
   * @return reference to stream
   */
  template <class Stream,
            typename = std::enable_if_t<Stream::is_encoder_stream>>
  Stream &operator<<(Stream &s, const NoData &) {
    return s;
  }

  /**
   * @brief decodes object of type Status from stream
   * @tparam Stream input stream type
   * @param s stream reference
   * @param v value to decode
   * @return reference to stream
   */
  template <class Stream,
            typename = std::enable_if_t<Stream::is_decoder_stream>>
  Stream &operator>>(Stream &s, NoData &) {
    return s;
  }

}  // namespace kagome::network

#endif  // KAGOME_CORE_NETWORK_TYPES_NO_DATA_HPP