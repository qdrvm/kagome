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

#include "application/impl/config_reader/error.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::application,
                            ConfigReaderError,
                            e) {
  using E = kagome::application::ConfigReaderError;
  switch (e) {
    case E::MISSING_ENTRY:
      return "A required entry is missing in the provided config file";
    case E::PARSER_ERROR:
      return "Internal parser error";
  }
  return "Unknown error";
}
