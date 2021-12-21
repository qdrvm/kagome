/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_SLOT_HPP
#define KAGOME_SLOT_HPP

namespace kagome::consensus {

  enum class SlotType {
    Primary,
    SecondaryPlain,
    SecondaryVRF,
  };

}

#endif  // KAGOME_SLOT_HPP
