/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>
#include <jsonrpc-lean/server.h>

#include "api/jrpc/jrpc_handle_batch.hpp"

using kagome::api::JrpcHandleBatch;

#define REQUEST(id) \
  R"({"jsonrpc":"2.0","method":"foo","id":)" #id R"(,"params":[]})"
#define RESPONSE(id) R"({"jsonrpc":"2.0","id":)" #id R"(,"result":0})"

struct JrpcHanldeBatchTest : ::testing::Test {
  jsonrpc::Server jsonrpc_handler_;
  jsonrpc::JsonFormatHandler format_handler_;
  void SetUp() override {
    jsonrpc_handler_.RegisterFormatHandler(format_handler_);
    auto &dispatcher = jsonrpc_handler_.GetDispatcher();
    dispatcher.AddMethod("foo", [] { return 0; });
  }
};

/**
 * @given single request
 * @when handle single request
 * @then single response is returned
 */
TEST_F(JrpcHanldeBatchTest, Single) {
  JrpcHandleBatch single(jsonrpc_handler_, REQUEST(0));
  EXPECT_EQ(single.response(), RESPONSE(0));
}

/**
 * @given batch request
 * @when handle batch request
 * @then batch response is returned
 */
TEST_F(JrpcHanldeBatchTest, Batch) {
  JrpcHandleBatch batch(jsonrpc_handler_, "[" REQUEST(1) "," REQUEST(2) "]");
  EXPECT_EQ(batch.response(), "[" RESPONSE(1) "," RESPONSE(2) "]");
}
