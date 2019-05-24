/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "primitives/parachain_host.hpp"

#include "scale/scale_encoder_stream.hpp"

namespace kagome::scale {
  // TODO(yuraz): PRE-152 update Relay << >> when it updates in substrate
  ScaleEncoderStream &operator<<(ScaleEncoderStream &s, const primitives::parachain::Relay &v) {
    return s;
  }
}
