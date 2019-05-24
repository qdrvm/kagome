/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "primitives/version.hpp"

#include "scale/scale_encoder_stream.hpp"

namespace kagome::scale {
  ScaleEncoderStream &operator<<(ScaleEncoderStream &s,
                                 const primitives::Version &v) {
    return s << std::string_view(v.spec_name) << std::string_view(v.impl_name)
             << v.authoring_version << v.impl_version << v.apis;
  }
}  // namespace kagome::scale
