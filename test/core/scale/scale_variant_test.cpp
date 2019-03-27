/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "common/result.hpp"
#include "scale/basic_stream.hpp"
#include <scale/variant.hpp>

using namespace kagome::common;
using namespace kagome::common::scale;

TEST(Scale, encodeVariant) {
    std::variant<uint8_t, uint32_t> v = static_cast<uint8_t>(1);
    Buffer out;
    variant::encodeVariant(v, out);
}