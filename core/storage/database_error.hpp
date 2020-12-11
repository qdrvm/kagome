/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_STORAGE_DATABASE_ERROR_DATABASE_ERROR_HPP
#define KAGOME_CORE_STORAGE_DATABASE_ERROR_DATABASE_ERROR_HPP

#include <outcome/outcome.hpp>

namespace kagome::storage {

  /**
   * @brief universal database interface error
   */
  enum class DatabaseError : int {
    OK = 0,
    NOT_FOUND = 1,
    CORRUPTION = 2,
    NOT_SUPPORTED = 3,
    INVALID_ARGUMENT = 4,
    IO_ERROR = 5,
    DB_PATH_NOT_CREATED = 6,

    UNKNOWN = 1000
  };
}  // namespace kagome::storage

OUTCOME_HPP_DECLARE_ERROR(kagome::storage, DatabaseError);

#endif  // KAGOME_CORE_STORAGE_DATABASE_ERROR_DATABASE_ERROR_HPP
