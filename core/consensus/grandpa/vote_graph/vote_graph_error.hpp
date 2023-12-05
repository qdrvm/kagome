/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <outcome/outcome.hpp>

namespace kagome::consensus::grandpa {

  enum VoteGraphError { RECEIVED_BLOCK_LESS_THAN_BASE = 1 };

}

OUTCOME_HPP_DECLARE_ERROR(kagome::consensus::grandpa, VoteGraphError);
