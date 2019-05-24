/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "primitives/extrinsic.hpp"

#include "scale/scale_encoder_stream.hpp"

namespace kagome::scale {
  ScaleEncoderStream &operator<<(ScaleEncoderStream &s, const primitives::Extrinsic &v) {
    return s.putBuffer(v.data);
  }

}  // namespace kagome::primitives
