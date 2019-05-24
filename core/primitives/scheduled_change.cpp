/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "primitives/scheduled_change.hpp"

#include "scale/scale_encoder_stream.hpp"

// it's going to be necessary later when refactoring scale
namespace kagome::scale {
  ScaleEncoderStream &operator<<(ScaleEncoderStream& s, const primitives::ScheduledChange &v) {
    return s << v.next_authorities << v.delay;
  }
}  // namespace kagome::scale
