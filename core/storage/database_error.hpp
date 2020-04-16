/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 *
 * Kagome is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * Kagome is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
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

    UNKNOWN = 1000
  };
}  // namespace kagome::storage

OUTCOME_HPP_DECLARE_ERROR(kagome::storage, DatabaseError);

#endif  // KAGOME_CORE_STORAGE_DATABASE_ERROR_DATABASE_ERROR_HPP
