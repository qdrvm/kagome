/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/db_error_translator/db_error_translator.hpp"

#include "storage/in_memory/in_memory_error.hpp"
#include "storage/leveldb/leveldb_error.hpp"

namespace kagome::storage {
  outcome::result<void> DbErrorTranslator::translateError(
      outcome::result<void> result) {
    auto &&error = result.error();
    if (error == LevelDBError::NOT_FOUND
        || error == InMemoryStorageError::NOT_FOUND) {
      return DbUnifiedError::KEY_NOT_FOUND;
    }

    // for now only NOT_FOUND error is processed

    return error;
  }
}  // namespace kagome::storage

OUTCOME_CPP_DEFINE_CATEGORY(kagome::storage, DbUnifiedError, e) {
  using kagome::storage::DbUnifiedError;
  switch (e) {
    case DbUnifiedError::KEY_NOT_FOUND:
      return "key not found in storage";
  }
  return "unknown DbUnifiedError error";
}
