/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "common/blob.hpp"

namespace kagome::common {

  // explicit instantiations for the most frequently used blobs
  template class Blob<8ul>;
  template class Blob<16ul>;
  template class Blob<32ul>;
  template class Blob<64ul>;

}  // namespace kagome::common
