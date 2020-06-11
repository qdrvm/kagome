/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "outcome/outcome.hpp"

namespace testutil {
  /**
   * @name Dummy error
   * @brief Special error for using instead special error and avoid need linkage
   * additional external library, i.e. in mock-object, call expectation, etc.
   * This provide several error codes for cases with different error.
   */
  enum class DummyError { ERROR = 1, ERROR_2, ERROR_3, ERROR_4, ERROR_5 };
}  // namespace testutil

OUTCOME_HPP_DECLARE_ERROR(testutil, DummyError);
