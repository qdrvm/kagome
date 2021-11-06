/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_ERROR
#define KAGOME_ERROR

#include "outcome/outcome.hpp"

namespace kagome {

  enum class Error {
    /// Special code for non error
    NO_ERROR = 0,
    /// Common code substitution for some non-important error
    OTHER_ERROR = 1,
    /// Common code substitution for some uncaught exception
    OTHER_EXCEPTION = 2
  };

}

OUTCOME_HPP_DECLARE_ERROR(kagome, Error)

#endif  // KAGOME_ERROR
