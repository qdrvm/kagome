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

#include "common/blob.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::common, BlobError, e) {
  using kagome::common::BlobError;

  switch(e) {
    case BlobError::INCORRECT_LENGTH:
      return "Input string has incorrect length, not matching the blob size";
  }

  return "Unknown error";
}

namespace kagome::common {

  // explicit instantiations for the most frequently used blobs
  template class Blob<8ul>;
  template class Blob<16ul>;
  template class Blob<32ul>;
  template class Blob<64ul>;

}  // namespace kagome::common
