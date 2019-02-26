/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_UNHEX_ERRORS_HPP
#define KAGOME_UNHEX_ERRORS_HPP

namespace kagome::common {

  /**
   * Errors that can happen during unhex
   */
  enum class UnhexError {
    kNotEnoughInput,   // given input contains odd number of symbols
    kNonHexInput,      // given input is not hexencoded string
    kWrongLengthInput  // given input string has unexpected size (i.e. when
                       // blob<size_>::fromHex(str) is invoked on string which
                       // does not represent byte array with size_ length)
  };

}  // namespace kagome::common

#endif  // KAGOME_UNHEX_ERRORS_HPP
