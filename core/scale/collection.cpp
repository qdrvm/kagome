/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "scale/collection.hpp"

namespace kagome::common::scale::collection {
  outcome::result<void> encodeBuffer(const Buffer &buf, Buffer &out) {
    OUTCOME_TRY(compact::encodeInteger(buf.size(), out));
    out.putBuffer(buf);
    return outcome::success();
  }
}  // namespace kagome::common::scale::collection
