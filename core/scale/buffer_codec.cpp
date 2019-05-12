/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "scale/buffer_codec.hpp"

#include "scale/collection.hpp"

namespace kagome::scale {

  outcome::result<common::Buffer> BufferScaleCodec::encode(
      const common::Buffer &val) {
    common::Buffer out;
    OUTCOME_TRY(collection::encodeCollection(val.toVector(), out));
    return out;
  }
}  // namespace kagome::scale
