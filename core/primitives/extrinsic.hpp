/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_PRIMITIVES_EXTRINSIC_HPP
#define KAGOME_PRIMITIVES_EXTRINSIC_HPP

#include "common/buffer.hpp"

namespace kagome::primitives {
  /**
   * @brief Extrinsic class represents extrinsic
   */
  class Extrinsic {
    using Buffer = kagome::common::Buffer;

   public:
    /**
     * @brief Extrinsic constructor
     * @param data extrinsic content
     */
    explicit Extrinsic(Buffer data);

    /**
     * @return extrinsic content as byte array
     */
    const Buffer &data() const;

   private:
    Buffer data_;  ///< extrinsic content as byte array
  };

}  // namespace kagome::primitives

#endif  // KAGOME_PRIMITIVES_EXTRINSIC_HPP
