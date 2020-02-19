/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/in_memory/in_memory_error.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::storage, InMemoryStorageError, e) {
  using kagome::storage::InMemoryStorageError;
  switch (e) {
    case InMemoryStorageError::NOT_FOUND:
      return "key not found in storage";
  }
  return "unknown InMemoryStorage error";
}
