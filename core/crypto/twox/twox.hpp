/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/blob.hpp"

namespace kagome::crypto {

  // TODO(warchant): PRE-357 refactor to span

  common::Hash64 make_twox64(common::BufferView buf);

  common::Hash128 make_twox128(common::BufferView buf);

  common::Hash256 make_twox256(common::BufferView buf);

}  // namespace kagome::crypto
