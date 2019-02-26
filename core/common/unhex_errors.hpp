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
  enum class UnhexErrors {
    kNotEnoughInput,  // given input contains odd number of symbols
    kNonHexInput      // given input is not hexencoded string
  };

}  // namespace kagome::common

#endif  // KAGOME_UNHEX_ERRORS_HPP
