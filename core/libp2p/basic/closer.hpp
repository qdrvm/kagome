/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CLOSER_HPP
#define KAGOME_CLOSER_HPP

#include <outcome/outcome.hpp>

namespace libp2p::basic {

class Closer {
 public:
  virtual ~Closer() = default;

  /**
   * @brief Function that is used to check if current object is closed.
   * @return true if closed, false otherwise
   */
  virtual bool isClosed() const = 0;

  /**
   * @brief Closes current object.
   */
  virtual outcome::result<void> close() = 0;
};

}  // namespace libp2p::basic

#endif //KAGOME_CLOSER_HPP
