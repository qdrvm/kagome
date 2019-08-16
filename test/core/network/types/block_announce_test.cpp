/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */
#include "network/types/block_announce.hpp"

#include <gmock/gmock.h>

using kagome::network::BlockAnnounce;

struct BlockAnnounceTest : public ::testing::Test {};

TEST_F(BlockAnnounceTest, EncodeSuccess) {}

TEST_F(BlockAnnounceTest, DecodeSuccess) {}
