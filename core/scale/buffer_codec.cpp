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

  outcome::result<common::Buffer> BufferScaleCodec::decode(
      common::ByteStream &stream) {
    OUTCOME_TRY(bytes, collection::decodeCollection<uint8_t>(stream));
    return common::Buffer(std::move(bytes));
  }
}  // namespace kagome::scale
