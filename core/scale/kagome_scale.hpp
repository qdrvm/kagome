/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <scale/scale.hpp>

namespace kagome::scale {

  using namespace ::scale;  // NOLINT(google-build-using-namespace)
  using ::scale::decode;
  using ::scale::encode;
  using ::scale::impl::memory::decode;
  using ::scale::impl::memory::DecoderFromSpan;
  using ::scale::impl::memory::encode;
  using ::scale::impl::memory::encoded_size;
  using ::scale::impl::memory::EncoderForCount;
  using ::scale::impl::memory::EncoderToVector;

}  // namespace kagome::scale
