/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "scale/collection.hpp"

namespace kagome::common::scale::collection {
  bool encodeBuffer(const Buffer &buf, Buffer &out) {
    auto result = compact::encodeInteger(buf.size(), out);
    if (!result) {
      return false;
    }

    out.putBuffer(buf);
    return true;
  }
}  // namespace kagome::common::scale::collection
