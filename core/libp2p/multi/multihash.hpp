/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_MULTIHASH_HPP
#define KAGOME_MULTIHASH_HPP

namespace libp2p::multi {
  /**
   * Special format of hash used in Libp2p. Allows to differentiate between
   * outputs of different hash functions. More
   * https://github.com/multiformats/multihash
   */
  class Multihash {};
}  // namespace libp2p::multi

#endif  // KAGOME_MULTIHASH_HPP
