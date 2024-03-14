/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <gmock/gmock-matchers.h>

#include "common/span_adl.hpp"

MATCHER_P(MatchSpan, expected, "") {
  return arg == SpanAdl{expected};
}
