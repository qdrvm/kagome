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

#include "primitives/inherent_data.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::primitives, InherentDataError, e) {
  using E = kagome::primitives::InherentDataError;
  switch (e) {
    case E::IDENTIFIER_ALREADY_EXISTS:
      return "This identifier already exists";
    case E::IDENTIFIER_DOES_NOT_EXIST:
      return "This identifier does not exist";
  }
  return "Unknow error";
}

namespace kagome::primitives {

  bool InherentData::operator==(
      const kagome::primitives::InherentData &rhs) const {
    return data == rhs.data;
  }

  bool InherentData::operator!=(
      const kagome::primitives::InherentData &rhs) const {
    return !operator==(rhs);
  }

}  // namespace kagome::primitives
