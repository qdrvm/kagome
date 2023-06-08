/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_ASSETS
#define KAGOME_ASSETS

#include <vector>

namespace kagome::assets {

  extern const char *const embedded_chainspec;
  extern const char *const embedded_chainspec_polkadot;
  extern const char *const embedded_chainspec_kusama;
  extern const char *const embedded_chainspec_rococo;
  extern const char *const embedded_chainspec_westend;
  extern const std::vector<std::pair<const char *, const char *>> embedded_keys;

}  // namespace kagome::assets

#endif  // KAGOME_ASSETS
