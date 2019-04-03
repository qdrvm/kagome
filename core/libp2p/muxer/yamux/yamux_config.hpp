/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_YAMUX_CONFIG_HPP
#define KAGOME_YAMUX_CONFIG_HPP

namespace libp2p::muxer {
  /**
   * Configuration used to create instance of Yamux multiplexer
   */
  struct YamuxConfig {
    bool is_server;
  };
}  // namespace libp2p::muxer

#endif  // KAGOME_YAMUX_CONFIG_HPP
