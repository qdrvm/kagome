/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "scale/collection.hpp"

namespace kagome::scale::collection {
  outcome::result<void> encodeBuffer(const common::Buffer &buf, common::Buffer &out) {
    OUTCOME_TRY(compact::encodeInteger(buf.size(), out));
    out.putBuffer(buf);
    return outcome::success();
  }
}  // namespace kagome::common::scale::collection
