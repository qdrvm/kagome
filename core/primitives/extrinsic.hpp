/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_PRIMITIVES_EXTRINSIC_HPP
#define KAGOME_PRIMITIVES_EXTRINSIC_HPP

#include "common/buffer.hpp"

using namespace kagome::common;

namespace kagome::primitives {
  /**
   * @brief Extrinsic class represents extrinsic
   */
  class Extrinsic {
   public:
    /**
     * @brief Extrinsic constructor
     * @param bytes extrinsic content
     */
    explicit Extrinsic(Buffer bytes);

    /**
     * @return extrinsic content as byte array
     */
    const Buffer &byteArray() const;

   private:
    Buffer byteArray_;  ///< extrinsic content as byte array
  };

}  // namespace kagome::primitives

#endif  // KAGOME_PRIMITIVES_EXTRINSIC_HPP
