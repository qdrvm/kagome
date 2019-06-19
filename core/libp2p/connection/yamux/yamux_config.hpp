/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_YAMUX_CONFIG_HPP
#define KAGOME_YAMUX_CONFIG_HPP

#include <cstdint>

namespace libp2p::connection {
  struct YamuxConfig {
    /// how much unconsumed data each stream can have stored locally
    uint32_t maximum_window_size = 1024 * 1024;  // 1 MB in bytes

    /// how much streams can be supported by Yamux at one time
    uint32_t maximum_streams = 1000;
  };
}  // namespace libp2p::connection

#endif  // KAGOME_YAMUX_CONFIG_HPP
