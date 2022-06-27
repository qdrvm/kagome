/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>
#include <jsonrpc-lean/server.h>

#include "api/jrpc/jrpc_handle_batch.hpp"

using kagome::api::JrpcHandleBatch;

struct JrpcHanldeBatchTest : ::testing::Test {
  jsonrpc::Server jsonrpc_handler_;
  jsonrpc::JsonFormatHandler format_handler_;
  void SetUp() override {
    jsonrpc_handler_.RegisterFormatHandler(format_handler_);
  }
};

TEST_F(JrpcHanldeBatchTest, Test) {
  auto &dispatcher = jsonrpc_handler_.GetDispatcher();
  dispatcher.AddMethod("foo", [] { return 0; });
#define REQ(id) R"({"jsonrpc":"2.0","method":"foo","id":)" #id R"(,"params":[]})"
#define RES(id) R"({"jsonrpc":"2.0","id":)" #id R"(,"result":0})"
  JrpcHandleBatch single(jsonrpc_handler_, REQ(0));
  EXPECT_EQ(single.response(), RES(0));
  JrpcHandleBatch batch(jsonrpc_handler_, "[" REQ(1) "," REQ(2) "]");
  EXPECT_EQ(batch.response(), "[" RES(1) "," RES(2) "]");
#undef REQ
#undef RES
}
