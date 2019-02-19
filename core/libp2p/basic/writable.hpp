/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_WRITABLE_HPP
#define KAGOME_WRITABLE_HPP

#include "libp2p/common/network_message.hpp"

namespace libp2p::basic {
  class Writable {
   public:
    /**
     * Write message
     * @param msg to be written
     */
    virtual void write(const common::NetworkMessage &msg) const = 0;
  };
}  // namespace libp2p::basic

#endif  // KAGOME_WRITABLE_HPP
