/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "network/scale_rpc_writer_libp2p.hpp"

#include <gtest/gtest.h>

class ScaleRpcWriterLibp2pTest : public testing::Test {};

TEST_F(ScaleRpcWriterLibp2pTest, WriteWithResponse) {}

TEST_F(ScaleRpcWriterLibp2pTest, WriteWithResponseErroredWrite) {}

TEST_F(ScaleRpcWriterLibp2pTest, WriteWithoutResponse) {}
