/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "testutil/libp2p/peer.hpp"

namespace testutil {

  PeerId testutil::randomPeerId() {
    PublicKey k;

    k.type = T::ED25519;
    for (auto i = 0u; i < 32u; i++) {
      k.data.putUint8(rand() & 0xff);
    }

    return PeerId::fromPublicKey(k).value();
  }

}  // namespace testutil
