/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "network/types/block_request.hpp"

#include <gmock/gmock.h>

using kagome::network::BlockAttributesBits;
using kagome::network::BlockRequest;
using kagome::network::Direction;

struct BlockRequestTest : public ::testing::Test {};

TEST_F(BlockRequestTest, EncodeSuccess) {}

TEST_F(BlockRequestTest, DecodeSuccess) {}
