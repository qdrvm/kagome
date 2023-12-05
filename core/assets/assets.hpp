/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <vector>

namespace kagome::assets {

  extern const char *const embedded_chainspec;
  extern const char *const embedded_chainspec_polkadot;
  extern const char *const embedded_chainspec_kusama;
  extern const char *const embedded_chainspec_rococo;
  extern const char *const embedded_chainspec_westend;
  extern const std::vector<std::pair<const char *, const char *>> embedded_keys;

}  // namespace kagome::assets
