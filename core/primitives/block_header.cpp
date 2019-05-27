/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "primitives/block_header.hpp"

#include "scale/scale_encoder_stream.hpp"

namespace kagome::scale {
  class ScaleEncoderStream;

  ScaleEncoderStream &operator<<(ScaleEncoderStream &s,
                                 const primitives::BlockHeader &bh) {
    return s << bh.parent_hash << bh.number << bh.state_root
             << bh.extrinsics_root << bh.digest;
  }
}  // namespace kagome::scale
