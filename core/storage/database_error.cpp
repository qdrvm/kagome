/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/database_error.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::storage, DatabaseError, e) {
  using E = kagome::storage::DatabaseError;
  switch (e) {
    case E::OK:
      return "success";
    case E::NOT_SUPPORTED:
      return "operation is not supported in database";
    case E::CORRUPTION:
      return "data corruption in database";
    case E::INVALID_ARGUMENT:
      return "invalid argument to database";
    case E::IO_ERROR:
      return "IO error in database";
    case E::NOT_FOUND:
      return "entry not found in database";
    case E::DB_PATH_NOT_CREATED:
      return "database path was not created";
    case E::STORAGE_GONE:
      return "database instance has been uninitialized";
    case E::UNKNOWN:
      break;
  }

  return "unknown error";
}
