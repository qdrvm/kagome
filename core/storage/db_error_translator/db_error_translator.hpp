/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_STORAGE_DB_ERROR_TRANSLATOR_DB_ERROR_TRANSLATOR_HPP
#define KAGOME_CORE_STORAGE_DB_ERROR_TRANSLATOR_DB_ERROR_TRANSLATOR_HPP

#include <outcome/outcome.hpp>

namespace kagome::storage {

  enum class DbUnifiedError: int {
    KEY_NOT_FOUND = 1, // key not found
  };

  /**
   * @class DbErrorTranslator translates db errors into suitable error codes
   * currently only NOT_FOUND error is translated
   */
  class DbErrorTranslator {
   public:
    outcome::result<void> translateError(outcome::result<void> result);
  };
}  // namespace kagome::storage

OUTCOME_HPP_DECLARE_ERROR(kagome::storage, DbUnifiedError);

#endif  // KAGOME_CORE_STORAGE_DB_ERROR_TRANSLATOR_DB_ERROR_TRANSLATOR_HPP
