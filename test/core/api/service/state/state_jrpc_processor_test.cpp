/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/service/state/state_jrpc_processor.hpp"

#include <unordered_map>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <boost/range/adaptor/transformed.hpp>

#include "api/service/state/requests/query_storage.hpp"  // for makeValue
#include "mock/core/api/jrpc/jrpc_server_mock.hpp"
#include "mock/core/api/service/state/state_api_mock.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"

using kagome::api::JRpcServer;
using kagome::api::JRpcServerMock;
using kagome::api::makeValue;
using kagome::api::StateApi;
using kagome::api::StateApiMock;
using kagome::api::state::StateJrpcProcessor;
using kagome::common::Buffer;
using kagome::common::BufferView;
using kagome::primitives::BlockHash;

using testing::_;
using testing::Invoke;
using testing::Return;
using testing::Truly;

auto operator==(const std::span<const Buffer> lhs,
                const std::span<const Buffer> rhs) {
  return std::equal(
      lhs.begin(), lhs.end(), rhs.begin(), [](const auto &le, const auto &re) {
        return std::equal(le.begin(), le.end(), re.begin());
      });
}

class StateJrpcProcessorTest : public testing::Test {
 public:
  enum struct CallType {
    kCallType_GetRuntimeVersion = 0,
    kCallType_SubscribeRuntimeVersion,
    kCallType_UnsubscribeRuntimeVersion,
    kCallType_GetKeysPaged,
    kCallType_GetStorage,
    kCallType_QueryStorage,
    kCallType_QueryStorageAt,
    kCallType_GetReadProof,
    kCallType_StorageSubscribe,
    kCallType_StorageUnsubscribe,
    kCallType_GetMetadata,
    kCallType_Call,
  };

 private:
  struct CallContext {
    JRpcServer::Method handler;
  };
  std::unordered_map<CallType, CallContext> call_contexts_;

 public:
  void SetUp() override {}

  void registerHandlers() {
    call_contexts_.clear();
    EXPECT_CALL(*server, registerHandler("state_call", _, _))
        .WillOnce(Invoke([&](auto &name, auto &&f, bool) {
          call_contexts_.emplace(std::make_pair(CallType::kCallType_Call,
                                                CallContext{.handler = f}));
        }));
    EXPECT_CALL(*server, registerHandler("state_getRuntimeVersion", _, _))
        .WillOnce(Invoke([&](auto &name, auto &&f, bool) {
          call_contexts_.emplace(
              std::make_pair(CallType::kCallType_GetRuntimeVersion,
                             CallContext{.handler = f}));
        }));
    EXPECT_CALL(*server, registerHandler("chain_getRuntimeVersion", _, _))
        .WillOnce(Invoke([&](auto &name, auto &&f, bool) {
          call_contexts_.emplace(
              std::make_pair(CallType::kCallType_SubscribeRuntimeVersion,
                             CallContext{.handler = f}));
        }));
    EXPECT_CALL(*server, registerHandler("state_subscribeRuntimeVersion", _, _))
        .WillOnce(Invoke([&](auto &name, auto &&f, bool) {
          call_contexts_.emplace(
              std::make_pair(CallType::kCallType_SubscribeRuntimeVersion,
                             CallContext{.handler = f}));
        }));
    EXPECT_CALL(*server,
                registerHandler("state_unsubscribeRuntimeVersion", _, _))
        .WillOnce(Invoke([&](auto &name, auto &&f, bool) {
          call_contexts_.emplace(
              std::make_pair(CallType::kCallType_UnsubscribeRuntimeVersion,
                             CallContext{.handler = f}));
        }));
    EXPECT_CALL(*server, registerHandler("state_getKeysPaged", _, _))
        .WillOnce(Invoke([&](auto &name, auto &&f, bool) {
          call_contexts_.emplace(std::make_pair(
              CallType::kCallType_GetKeysPaged, CallContext{.handler = f}));
        }));
    EXPECT_CALL(*server, registerHandler("state_getStorage", _, _))
        .WillOnce(Invoke([&](auto &name, auto &&f, bool) {
          call_contexts_.emplace(std::make_pair(CallType::kCallType_GetStorage,
                                                CallContext{.handler = f}));
        }));
    EXPECT_CALL(*server, registerHandler("state_getStorageAt", _, _))
        .WillOnce(Invoke([&](auto &name, auto &&f, bool) {
          call_contexts_.emplace(std::make_pair(CallType::kCallType_GetStorage,
                                                CallContext{.handler = f}));
        }));
    EXPECT_CALL(*server, registerHandler("state_queryStorage", _, _))
        .WillOnce(Invoke([&](auto &name, auto &&f, bool) {
          call_contexts_.emplace(std::make_pair(
              CallType::kCallType_QueryStorage, CallContext{.handler = f}));
        }));
    EXPECT_CALL(*server, registerHandler("state_queryStorageAt", _, _))
        .WillOnce(Invoke([&](auto &name, auto &&f, bool) {
          call_contexts_.emplace(std::make_pair(
              CallType::kCallType_QueryStorageAt, CallContext{.handler = f}));
        }));
    EXPECT_CALL(*server, registerHandler("state_getReadProof", _, _))
        .WillOnce(Invoke([&](auto &name, auto &&f, bool) {
          call_contexts_.emplace(std::make_pair(
              CallType::kCallType_GetReadProof, CallContext{.handler = f}));
        }));
    EXPECT_CALL(*server, registerHandler("state_subscribeStorage", _, _))
        .WillOnce(Invoke([&](auto &name, auto &&f, bool) {
          call_contexts_.emplace(std::make_pair(
              CallType::kCallType_StorageSubscribe, CallContext{.handler = f}));
        }));
    EXPECT_CALL(*server, registerHandler("state_unsubscribeStorage", _, _))
        .WillOnce(Invoke([&](auto &name, auto &&f, bool) {
          call_contexts_.emplace(
              std::make_pair(CallType::kCallType_StorageUnsubscribe,
                             CallContext{.handler = f}));
        }));
    EXPECT_CALL(*server, registerHandler("state_getMetadata", _, _))
        .WillOnce(Invoke([&](auto &name, auto &&f, bool) {
          call_contexts_.emplace(std::make_pair(CallType::kCallType_GetMetadata,
                                                CallContext{.handler = f}));
        }));
    processor.registerHandlers();
  }

  template <typename... Args>
  auto execute(CallType method, Args &&...args) {
    return call_contexts_[method].handler(std::forward<Args>(args)...);
  }

  std::shared_ptr<StateApiMock> state_api = std::make_shared<StateApiMock>();
  std::shared_ptr<JRpcServerMock> server = std::make_shared<JRpcServerMock>();
  StateJrpcProcessor processor{server, state_api};
};

/**
 * @given a request of state_getStorage with a valid param
 * @when processing it
 * @then the request is successfully processed and the response is valid
 */
TEST_F(StateJrpcProcessorTest, ProcessRequest) {
  auto expected_result = "ABCDEF"_hex2buf;
  auto key = "01234567"_hex2buf;
  EXPECT_CALL(*state_api, getStorage(key.view()))
      .WillOnce(Return(expected_result));

  registerHandlers();

  jsonrpc::Request::Parameters params{"0x01234567"};
  auto result_hex = execute(CallType::kCallType_GetStorage, params).AsString();
  EXPECT_OUTCOME_TRUE(result_vec, kagome::common::unhexWith0x(result_hex));
  ASSERT_EQ(expected_result.asVector(), result_vec);
}

/**
 * @given a request of state_getStorage with two valid params
 * @when processing it
 * @then the request is successfully processed and the response is valid
 */
TEST_F(StateJrpcProcessorTest, ProcessAnotherRequest) {
  auto expected_result = "ABCDEF"_hex2buf;

  auto key = "01234567"_hex2buf;
  EXPECT_CALL(*state_api, getStorageAt(key.view(), "010203"_hash256))
      .WillOnce(Return(expected_result));

  registerHandlers();

  jsonrpc::Request::Parameters params{"0x01234567",
                                      "0x" + ("010203"_hash256).toHex()};
  auto result_hex = execute(CallType::kCallType_GetStorage, params).AsString();
  EXPECT_OUTCOME_TRUE(result_vec, kagome::common::unhexWith0x(result_hex));
  ASSERT_EQ(expected_result.asVector(), result_vec);
}

/**
 * @given set of keys and a block
 * @when querying storage changes for the given key set on a block range from
 * the given to the latest by queryStorage in State API
 * @then method call returns a JSON object, which data matches the expected call
 * result
 */
TEST_F(StateJrpcProcessorTest, ProcessQueryStorage) {
  // GIVEN
  std::vector<Buffer> keys{"key1"_buf, "key2"_buf, "key3"_buf};
  BlockHash from{"from"_hash256};
  std::vector<StateApi::StorageChangeSet> res{StateApi::StorageChangeSet{
      from, {StateApi::StorageChangeSet::Change{"key1"_buf, "42"_buf}}}};
  auto if_keys = Truly([&](const auto &x) {
    return std::equal(x.begin(), x.end(), keys.begin());
  });
  EXPECT_CALL(*state_api,
              queryStorage(if_keys, from, std::optional<BlockHash>{}))
      .WillOnce(Return(outcome::success(res)));

  registerHandlers();

  jsonrpc::Value::Array keys_json;
  std::transform(keys.begin(),
                 keys.end(),
                 std::back_inserter(keys_json),
                 [](auto &buffer) { return "0x" + buffer.toHex(); });
  jsonrpc::Request::Parameters params{keys_json, "0x" + from.toHex()};
  // WHEN
  auto result = execute(CallType::kCallType_QueryStorage, params);
  auto expected_json = makeValue(res).AsArray();
  // THEN
  ASSERT_EQ(expected_json.size(), result.AsArray().size());
  for (auto expected = expected_json.cbegin(),
            received = result.AsArray().begin();
       expected != expected_json.end();
       expected++, received++) {
    ASSERT_EQ(expected->AsStruct().at("block").AsString(),
              received->AsStruct().at("block").AsString());
    auto json_tuple_to_pair = [](auto &value) {
      auto &tuple = value.AsArray();
      return std::pair(tuple[0].AsString(), tuple[1].AsString());
    };
    auto e = expected->AsStruct().at("changes").AsArray()
           | boost::adaptors::transformed(json_tuple_to_pair);
    auto r = received->AsStruct().at("changes").AsArray()
           | boost::adaptors::transformed(json_tuple_to_pair);
    ASSERT_EQ(e, r);
  }
}

TEST_F(StateJrpcProcessorTest, ProcessQueryStorageAt) {
  // GIVEN
  std::vector<Buffer> keys{"key1"_buf, "key2"_buf, "key3"_buf};
  BlockHash at{"at"_hash256};
  std::vector<StateApi::StorageChangeSet> res{StateApi::StorageChangeSet{
      at, {StateApi::StorageChangeSet::Change{"key1"_buf, "42"_buf}}}};

  auto if_keys = Truly(
      [&](auto x) { return std::equal(x.begin(), x.end(), keys.begin()); });
  EXPECT_CALL(*state_api, queryStorageAt(if_keys, std::make_optional(at)))
      .WillOnce(Return(outcome::success(res)));

  registerHandlers();

  jsonrpc::Value::Array keys_json;
  std::transform(keys.begin(),
                 keys.end(),
                 std::back_inserter(keys_json),
                 [](auto &buffer) { return "0x" + buffer.toHex(); });
  jsonrpc::Request::Parameters params{keys_json, "0x" + at.toHex()};
  // WHEN
  auto result = execute(CallType::kCallType_QueryStorageAt, params);
  auto expected_json = makeValue(res).AsArray();
  // THEN
  ASSERT_EQ(expected_json.size(), result.AsArray().size());
  ASSERT_EQ(expected_json.size(), 1);

  ASSERT_EQ(expected_json[0].AsStruct().at("block").AsString(),
            result.AsArray()[0].AsStruct().at("block").AsString());
  auto json_tuple_to_pair = [](auto &value) {
    auto &tuple = value.AsArray();
    return std::pair(tuple[0].AsString(), tuple[1].AsString());
  };
  auto e = expected_json[0].AsStruct().at("changes").AsArray()
         | boost::adaptors::transformed(json_tuple_to_pair);
  auto r = result.AsArray()[0].AsStruct().at("changes").AsArray()
         | boost::adaptors::transformed(json_tuple_to_pair);
  ASSERT_EQ(e, r);
}

/**
 * @given a request of state_getStorage with invalid params
 * @when processing it
 * @then InvalidParametersFault exception is thrown
 */

TEST_F(StateJrpcProcessorTest, InvalidParams) {
  registerHandlers();

  jsonrpc::Request::Parameters params;
  params.push_back(jsonrpc::Value{0});
  params.push_back(0);
  ASSERT_THROW(execute(CallType::kCallType_GetStorage, params).AsArray(),
               jsonrpc::InvalidParametersFault);
}

/**
 * @given a request of state_getRuntimeVersion with a valid param
 * @when processing it
 * @then the request is successfully processed and the response is valid
 */
TEST_F(StateJrpcProcessorTest, ProcessGetVersionRequest) {
  kagome::primitives::Version test_version{.spec_name = "dummy_sn",
                                           .impl_name = "dummy_in",
                                           .authoring_version = 0x101,
                                           .spec_version = 0x111,
                                           .impl_version = 0x202,
                                           .apis = {}};

  std::optional<kagome::primitives::BlockHash> hash = std::nullopt;
  EXPECT_CALL(*state_api, getRuntimeVersion(hash))
      .WillOnce(Return(test_version));

  registerHandlers();

  jsonrpc::Request::Parameters params{};
  auto result =
      execute(CallType::kCallType_GetRuntimeVersion, params).AsStruct();

  ASSERT_EQ(result["authoringVersion"].AsInteger64(),
            test_version.authoring_version);
  ASSERT_EQ(result["specName"].AsString(), test_version.spec_name);
  ASSERT_EQ(result["implName"].AsString(), test_version.impl_name);
  ASSERT_EQ(result["specVersion"].AsInteger64(), test_version.spec_version);
  ASSERT_EQ(result["implVersion"].AsInteger64(), test_version.impl_version);
}

/**
 * @given a request of state_subscribeStorage with a valid param
 * @when processing it
 * @then the request is successfully processed and the response is valid
 */
TEST_F(StateJrpcProcessorTest, ProcessSubscribeStorage) {
  uint32_t val = 10;
  std::vector<Buffer> keys;
  keys.emplace_back(kagome::common::unhexWith0x("0x1011").value());
  keys.emplace_back(kagome::common::unhexWith0x("0x2002").value());

  EXPECT_CALL(*state_api, subscribeStorage(keys)).WillOnce(Return(val));

  registerHandlers();

  jsonrpc::Value::Array data;
  data.emplace_back("0x1011");
  data.emplace_back("0x2002");

  jsonrpc::Request::Parameters params{data};
  auto result =
      execute(CallType::kCallType_StorageSubscribe, params).AsInteger32();

  ASSERT_EQ(result, val);
}

/**
 * @given a request of state_unsubscribeStorage with a valid param
 * @when processing it
 * @then the request is successfully processed and the response is valid
 */
TEST_F(StateJrpcProcessorTest, ProcessUnsubscribeStorage) {
  int32_t subscription_id = 10;
  EXPECT_CALL(*state_api,
              unsubscribeStorage(std::vector<uint32_t>{
                  static_cast<uint32_t>(subscription_id)}))
      .WillOnce(Return(true));

  registerHandlers();

  jsonrpc::Request::Parameters params{jsonrpc::Value(subscription_id)};
  execute(CallType::kCallType_StorageUnsubscribe, params);
}
