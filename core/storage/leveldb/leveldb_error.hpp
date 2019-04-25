/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_LEVELDB_ERROR_HPP
#define KAGOME_LEVELDB_ERROR_HPP

#include <outcome/outcome-register.hpp>

namespace kagome::storage {

  /**
   * @brief LevelDB returns those type of errors, as described in
   * <leveldb/status.h>, Status::Code (it is private)
   */
  enum class LevelDBError {
    kOk = 0,
    kNotFound = 1,
    kCorruption = 2,
    kNotSupported = 3,
    kInvalidArgument = 4,
    kIOError = 5,

    kUnknown = 1000
  };

}  // namespace kagome::storage

OUTCOME_HPP_DECLARE_ERROR(kagome::storage, LevelDBError);

#endif  // KAGOME_LEVELDB_ERROR_HPP
