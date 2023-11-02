/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "common/blob.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::common, BlobError, e) {
  using kagome::common::BlobError;

  switch (e) {
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
