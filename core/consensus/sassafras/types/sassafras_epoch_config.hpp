/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

namespace kagome::consensus::sassafras {

  class SassafrasEpochConfig {
    attempts_number : SASSAFRAS_TICKETS_MAX_ATTEMPTS_NUMBER,
                      redundancy_factor : SASSAFRAS_TICKETS_REDUNDANCY_FACTOR,
  };

}  // namespace kagome::consensus::sassafras
