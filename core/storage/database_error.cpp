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
    case E::UNKNOWN:
      break;
  }

  return "unknown error";
}
