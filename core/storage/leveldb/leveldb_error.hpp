/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_LEVELDB_ERROR_HPP
#define KAGOME_LEVELDB_ERROR_HPP

#include <outcome/outcome.hpp>

namespace kagome::storage {

  /**
   * @brief LevelDB returns those type of errors, as described in
   * <leveldb/status.h>, Status::Code (it is private)
   */
  enum class LevelDBError {
    OK = 0,
    NOT_FOUND = 1,
    CORRUPTION = 2,
    NOT_SUPPORTED = 3,
    INVALID_ARGUMENT = 4,
    IO_ERROR = 5,

    UNKNOWN = 1000
  };

}  // namespace kagome::storage

OUTCOME_HPP_DECLARE_ERROR(kagome::storage, LevelDBError);

#endif  // KAGOME_LEVELDB_ERROR_HPP
