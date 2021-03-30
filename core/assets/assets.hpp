/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_ASSETS
#define KAGOME_ASSETS

#include <vector>

namespace kagome::assets {

#ifdef USE_KAGOME_EMBEDDINGS
  extern const char *const embedded_chainspec;
  extern const std::vector<std::pair<const char *, const char *>> embedded_keys;
#endif  // USE_KAGOME_EMBEDDINGS

}  // namespace kagome::assets

#endif  // KAGOME_ASSETS
