/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_WRITABLE_HPP
#define KAGOME_WRITABLE_HPP

namespace libp2p {
  namespace basic_interfaces {
    class Writable {
     public:
      /**
       * Write message
       * @param msg to be written
       */
      virtual void write(const common::NetworkMessage &msg) const = 0;
    };
  }  // namespace basic_interfaces
}  // namespace libp2p

#endif  // KAGOME_WRITABLE_HPP
