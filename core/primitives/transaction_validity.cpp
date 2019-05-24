/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "primitives/transaction_validity.hpp"

#include "scale/scale_encoder_stream.hpp"

namespace kagome::scale {

  ScaleEncoderStream &operator<<(ScaleEncoderStream &s,
                                 const primitives::Invalid &v) {
    return s << v.error_;
  }

  ScaleEncoderStream &operator<<(ScaleEncoderStream &s,
                                 const primitives::Unknown &v) {
    return s << v.error_;
  }

  ScaleEncoderStream &operator<<(ScaleEncoderStream &s,
                                 const primitives::Valid &v) {
    return s << v.priority_ << v.requires_ << v.provides_ << v.longevity_;
  }

}  // namespace kagome::scale
