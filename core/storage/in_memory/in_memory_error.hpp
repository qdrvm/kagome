/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_STORAGE_IN_MEMORY_IN_MEMORY_ERROR_HPP
#define KAGOME_CORE_STORAGE_IN_MEMORY_IN_MEMORY_ERROR_HPP

#include <outcome/outcome.hpp>

namespace kagome::storage {

  enum class InMemoryStorageError : int {
    NOT_FOUND = 1,  // key not found
  };
}

OUTCOME_HPP_DECLARE_ERROR(kagome::storage, InMemoryStorageError);

#endif  // KAGOME_CORE_STORAGE_IN_MEMORY_IN_MEMORY_ERROR_HPP
