/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "dispute_coordinator/participation/impl/queues_impl.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::dispute, QueueError, e) {
  using E = kagome::dispute::QueueError;
  switch (e) {
    case E::BestEffortFull:
      return "Request could not be queued, because best effort queue was "
             "already full.";
    case E::PriorityFull:
      return "Request could not be queued, because priority queue was already "
             "full.";
  }
  return "unknown error (dispute::QueueError)";
}

namespace kagome::dispute {

  //

}
