/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/leveldb/leveldb_error.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::storage, LevelDBError, e) {
  using E = kagome::storage::LevelDBError;
  switch (e) {
    case E::kOk:
      return "success";
    case E::kNotSupported:
      return "operation is not supported";
    case E::kCorruption:
      return "data corruption";
    case E::kInvalidArgument:
      return "invalid argument";
    case E::kIOError:
      return "IO error";
    case E::kNotFound:
      return "not found";
    case E::kUnknown:
      break;
  }

  return "unknown error";
}
