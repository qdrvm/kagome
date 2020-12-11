/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/database_error.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::storage, DatabaseError, e) {
  using E = kagome::storage::DatabaseError;
  switch (e) {
    case E::OK:
      return "success";
    case E::NOT_SUPPORTED:
      return "operation is not supported";
    case E::CORRUPTION:
      return "data corruption";
    case E::INVALID_ARGUMENT:
      return "invalid argument";
    case E::IO_ERROR:
      return "IO error";
    case E::NOT_FOUND:
      return "not found";
    case E::DB_PATH_NOT_CREATED:
      return "database path was not created";
    case E::UNKNOWN:
      break;
  }

  return "unknown error";
}
