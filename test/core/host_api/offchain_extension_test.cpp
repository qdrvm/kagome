/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "host_api/impl/offchain_extension.hpp"

#include <gtest/gtest.h>

#include "mock/core/offchain/offchain_persistent_storage_mock.hpp"
#include "mock/core/offchain/offchain_worker_mock.hpp"
#include "mock/core/offchain/offchain_worker_pool_mock.hpp"
#include "mock/core/runtime/memory_provider_mock.hpp"
#include "mock/core/runtime/trie_storage_provider_mock.hpp"
#include "mock/core/storage/trie/polkadot_trie_cursor_mock.h"
#include "mock/core/storage/trie/trie_batches_mock.hpp"
#include "offchain/types.hpp"
#include "runtime/ptr_size.hpp"
#include "scale/encode_append.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"
#include "testutil/outcome/dummy_error.hpp"
#include "testutil/prepare_loggers.hpp"
#include "testutil/runtime/memory.hpp"

using kagome::common::Buffer;
using kagome::common::BufferView;
using kagome::host_api::OffchainExtension;
using kagome::host_api::OffchainExtensionConfig;
using kagome::offchain::Failure;
using kagome::offchain::HttpError;
using kagome::offchain::HttpMethod;
using kagome::offchain::HttpStatus;
using kagome::offchain::OffchainPersistentStorageMock;
using kagome::offchain::OffchainWorkerMock;
using kagome::offchain::OffchainWorkerPoolMock;
using kagome::offchain::OpaqueNetworkState;
using kagome::offchain::RandomSeed;
using kagome::offchain::RequestId;
using kagome::offchain::Result;
using kagome::offchain::StorageType;
using kagome::offchain::Success;
using kagome::offchain::Timestamp;
using kagome::primitives::Extrinsic;
using kagome::runtime::MemoryProviderMock;
using kagome::runtime::PtrSize;
using kagome::runtime::TestMemory;
using kagome::runtime::TrieStorageProviderMock;
using kagome::runtime::WasmI32;
using kagome::runtime::WasmOffset;
using kagome::runtime::WasmPointer;
using kagome::runtime::WasmSize;
using kagome::runtime::WasmSpan;

using ::testing::_;
using ::testing::Invoke;
using ::testing::Return;
using ::testing::Truly;

namespace kagome::offchain {
  std::ostream &operator<<(std::ostream &s, const Success &) {
    return s << "{Success}";
  }
  std::ostream &operator<<(std::ostream &s, const Failure &) {
    return s << "{Failure}";
  }
  std::ostream &operator<<(std::ostream &s, const HttpError &e) {
    switch (e) {
      case HttpError::Timeout:
        return s << "{Timeout}";
      case HttpError::IoError:
        return s << "{IoError}";
      case HttpError::InvalidId:
        return s << "{InvalidId}";
    }
    return s;
  }
  std::ostream &operator<<(std::ostream &s, const OpaqueNetworkState &ons) {
    return s << "{peer_id=" << ons.peer_id.toBase58() << ", "
             << ons.address.size() << " addresses}";
  }
}  // namespace kagome::offchain

class OffchainExtensionTest : public ::testing::Test {
 public:
  static void SetUpTestCase() {
    testutil::prepareLoggers();
  }

  void SetUp() override {
    memory_provider_ = std::make_shared<MemoryProviderMock>();
    EXPECT_CALL(*memory_provider_, getCurrentMemory())
        .WillRepeatedly(Return(std::ref(memory_.memory)));
    offchain_storage_ = std::make_shared<OffchainPersistentStorageMock>();
    offchain_worker_ = std::make_shared<OffchainWorkerMock>();
    offchain_worker_pool_ = std::make_shared<OffchainWorkerPoolMock>();
    EXPECT_CALL(*offchain_worker_pool_, getWorker())
        .WillRepeatedly(Return(offchain_worker_));
    offchain_extension_ = std::make_shared<OffchainExtension>(
        config_, memory_provider_, offchain_storage_, offchain_worker_pool_);
  }

 protected:
  OffchainExtensionConfig config_ = {true};
  std::shared_ptr<OffchainPersistentStorageMock> offchain_storage_;
  TestMemory memory_;
  std::shared_ptr<MemoryProviderMock> memory_provider_;
  std::shared_ptr<OffchainExtension> offchain_extension_;
  std::shared_ptr<OffchainWorkerMock> offchain_worker_;
  std::shared_ptr<OffchainWorkerPoolMock> offchain_worker_pool_;

  static constexpr uint32_t kU32Max = std::numeric_limits<uint32_t>::max();
};

/// For the tests where it is needed to check a valid behaviour no matter if
/// success or failure was returned by outcome::result
class OutcomeParameterizedTest
    : public OffchainExtensionTest,
      public ::testing::WithParamInterface<outcome::result<void>> {};

class BinaryParameterizedTest : public OffchainExtensionTest,
                                public ::testing::WithParamInterface<WasmI32> {
};

class TernaryParametrizedTest : public OffchainExtensionTest,
                                public ::testing::WithParamInterface<WasmI32> {
};

class HttpMethodsParametrizedTest
    : public OffchainExtensionTest,
      public ::testing::WithParamInterface<Buffer> {};

class HttpResultParametrizedTest
    : public OffchainExtensionTest,
      public ::testing::WithParamInterface<Result<uint32_t, HttpError>> {};

/**
 * @when ext_offchain_is_validator_version_1 is invoked on OffchainExtension
 * @then 1 returned if offchain is validator, 0 otherwise
 */
TEST_P(BinaryParameterizedTest, IsValidator) {
  EXPECT_CALL(*offchain_worker_, isValidator()).WillOnce(Return(GetParam()));
  ASSERT_EQ(GetParam(),
            offchain_extension_->ext_offchain_is_validator_version_1());
}

INSTANTIATE_TEST_SUITE_P(Instance,
                         BinaryParameterizedTest,
                         testing::Values(0, 1));

/**
 * @given extrinsic
 * @when ext_offchain_submit_transaction_version_1 is invoked on
 * OffchainExtension with extrinsic
 * @then extrinsic is fetched from parameter and submitted as transaction
 */
TEST_F(OffchainExtensionTest, SubmitTransaction) {
  Extrinsic xt{"data_buffer"_buf};

  EXPECT_CALL(*offchain_worker_, submitTransaction(_))
      .WillOnce(Return(Success{}));

  memory_[offchain_extension_->ext_offchain_submit_transaction_version_1(
      memory_.encode(xt))];
}

/**
 * @when ext_offchain_network_state_version_1 is invoked on
 * OffchainExtension
 * @then Returns network state
 */
TEST_F(OffchainExtensionTest, NetworkState) {
  Result<OpaqueNetworkState, Failure> ons = OpaqueNetworkState();

  EXPECT_CALL(*offchain_worker_, networkState()).WillOnce(Return(ons));

  memory_[offchain_extension_->ext_offchain_network_state_version_1()];
}

/**
 * @when ext_offchain_timestamp_version_1 is invoked on
 * OffchainExtension
 * @then Returns current timestamp
 */
TEST_F(OffchainExtensionTest, Timestamp) {
  Timestamp result{300000};

  EXPECT_CALL(*offchain_worker_, timestamp()).WillOnce(Return(result));

  ASSERT_EQ(result, offchain_extension_->ext_offchain_timestamp_version_1());
}

/**
 * @given deadline
 * @when ext_offchain_sleep_until_version_1 is invoked on
 * OffchainExtension
 * @then Worker sleeps until deadline
 */
TEST_F(OffchainExtensionTest, SleepUntil) {
  Timestamp deadline{300000};

  EXPECT_CALL(*offchain_worker_, sleepUntil(deadline)).WillOnce(Return());

  offchain_extension_->ext_offchain_sleep_until_version_1(deadline);
}

/**
 * @when ext_offchain_random_seed_version_1 is invoked on
 * OffchainExtension
 * @then Returns random seed, based on local time
 */
TEST_F(OffchainExtensionTest, RandomSeed) {
  RandomSeed result;

  EXPECT_CALL(*offchain_worker_, randomSeed()).WillOnce(Return(result));

  ASSERT_EQ(memory_.memory
                .view(offchain_extension_->ext_offchain_random_seed_version_1(),
                      result.size())
                .value(),
            SpanAdl{result});
}

/**
 * @given storage type, key, value
 * @when ext_offchain_local_storage_set_version_1 is invoked on
 * OffchainExtension
 * @then Attempts to write value into local storage
 */
TEST_P(TernaryParametrizedTest, LocalStorageSet) {
  Buffer key(8, 'k');
  Buffer value(8, 'v');
  EXPECT_CALL(*offchain_worker_, localStorageSet(_, key.view(), value));
  offchain_extension_->ext_offchain_local_storage_set_version_1(
      GetParam(), memory_[key], memory_[value]);
}

/**
 * @given storage type, key
 * @when ext_offchain_local_storage_clear_version_1 is invoked on
 * OffchainExtension
 * @then Attempts to remove value from local storage
 */
TEST_P(TernaryParametrizedTest, LocalStorageClear) {
  Buffer key(8, 'k');
  EXPECT_CALL(*offchain_worker_, localStorageClear(_, key.view()));
  offchain_extension_->ext_offchain_local_storage_clear_version_1(GetParam(),
                                                                  memory_[key]);
}

/**
 * @given storage type, key, expected value, value
 * @when ext_offchain_local_storage_compare_and_set_version_1 is invoked on
 * OffchainExtension
 * @then Attempts CAS on local storage
 */
TEST_P(TernaryParametrizedTest, LocalStorageCAS) {
  Buffer key(8, 'k');
  Buffer value(8, 'v');
  EXPECT_CALL(*offchain_worker_,
              localStorageCompareAndSet(
                  _, key.view(), std::optional<BufferView>{}, value))
      .WillOnce(Return(true));
  offchain_extension_->ext_offchain_local_storage_compare_and_set_version_1(
      GetParam(), memory_[key], memory_.encode(std::nullopt), memory_[value]);
}

/**
 * @given storage type, key
 * @when ext_offchain_local_storage_get_version_1 is invoked on
 * OffchainExtension with storage type and key
 * @then Attempts to return value from local storage
 */
TEST_P(TernaryParametrizedTest, LocalStorageGet) {
  Buffer key(8, 'k');
  auto result = "some_result"_buf;

  EXPECT_CALL(*offchain_worker_, localStorageGet(_, key.view()))
      .WillOnce(Return(outcome::success(result)));

  ASSERT_EQ(
      memory_
          .decode<std::optional<Buffer>>(
              offchain_extension_->ext_offchain_local_storage_get_version_1(
                  GetParam(), memory_[key]))
          .value(),
      result);
}

INSTANTIATE_TEST_SUITE_P(Instance,
                         TernaryParametrizedTest,
                         testing::Values(1, 2));

/**
 * @given method, uri, meta
 * @when ext_offchain_http_request_start_version_1 is invoked on
 * OffchainExtension
 * @then Attempts to start request on uri with given method. Meta is reserved.
 */
TEST_P(HttpMethodsParametrizedTest, HttpRequestStart) {
  Buffer method = GetParam();
  Buffer uri = "name"_buf;
  Buffer meta = "value"_buf;
  Result<RequestId, Failure> result{22};

  EXPECT_CALL(*offchain_worker_, httpRequestStart(_, uri.toString(), meta))
      .WillOnce(Return(result));

  ASSERT_EQ(memory_.decode<decltype(result)>(
                offchain_extension_->ext_offchain_http_request_start_version_1(
                    memory_[method], memory_[uri], memory_[meta])),
            result);
}

INSTANTIATE_TEST_SUITE_P(Instance,
                         HttpMethodsParametrizedTest,
                         testing::Values("Post"_buf,
                                         "Get"_buf,
                                         "Undefined"_buf));

/**
 * @given request_id, name, value
 * @when ext_offchain_http_request_add_header_version_1 is invoked on
 * OffchainExtension
 * @then Attempts to add header name:value to request
 */
TEST_F(OffchainExtensionTest, HttpRequestAddHeader) {
  RequestId id{22};
  Buffer name = "name"_buf;
  Buffer value = "value"_buf;
  Result<Success, Failure> result{Success{}};

  EXPECT_CALL(*offchain_worker_,
              httpRequestAddHeader(id, name.toString(), value.toString()))
      .WillOnce(Return(result));

  ASSERT_TRUE(memory_
                  .decode<decltype(result)>(
                      offchain_extension_
                          ->ext_offchain_http_request_add_header_version_1(
                              id, memory_[name], memory_[value]))
                  .isSuccess());
}

/**
 * @given request_id, chunk_ptr, deadline
 * @when ext_offchain_http_request_write_body_version_1 is invoked on
 * OffchainExtension
 * @then Attempts to write request body to chunk, returns result
 */
TEST_F(OffchainExtensionTest, HttpRequestWriteBody) {
  RequestId id{22};
  Buffer chunk{8, 'c'};
  Timestamp deadline{300000};
  auto deadline_opt = std::make_optional(deadline);

  Result<Success, HttpError> result{Success{}};

  EXPECT_CALL(*offchain_worker_, httpRequestWriteBody(id, chunk, deadline_opt))
      .WillOnce(Return(result));

  ASSERT_TRUE(memory_
                  .decode<decltype(result)>(
                      offchain_extension_
                          ->ext_offchain_http_request_write_body_version_1(
                              id, memory_[chunk], memory_.encode(deadline_opt)))
                  .isSuccess());
}

/**
 * @given request_ids, deadline
 * @when ext_offchain_http_response_wait_version_1 is invoked on
 * OffchainExtension
 * @then Waits for response of listed requests until deadline and returns
 * HttpResults
 */
TEST_F(OffchainExtensionTest, HttpResponseWait) {
  std::vector<RequestId> ids{22, 23};
  Timestamp deadline{300000};
  auto deadline_opt = std::make_optional(deadline);

  std::vector<HttpStatus> result{200};

  EXPECT_CALL(*offchain_worker_, httpResponseWait(ids, deadline_opt))
      .WillOnce(Return(result));

  ASSERT_EQ(memory_.decode<decltype(result)>(
                offchain_extension_->ext_offchain_http_response_wait_version_1(
                    memory_.encode(ids), memory_.encode(deadline_opt))),
            result);
}

/**
 * @given request_id
 * @when ext_offchain_http_response_headers_version_1 is invoked on
 * OffchainExtension with request_id
 * @then returns request headers
 */
TEST_F(OffchainExtensionTest, HttpResponseHeaders) {
  WasmI32 request_id = 22;
  std::vector<std::pair<std::string, std::string>> headers{{"a", "A"},
                                                           {"b", "B"}};
  EXPECT_CALL(*offchain_worker_, httpResponseHeaders(request_id))
      .WillOnce(Return(headers));

  ASSERT_EQ(
      memory_.decode<decltype(headers)>(
          offchain_extension_->ext_offchain_http_response_headers_version_1(
              request_id)),
      headers);
}

/**
 * @given request_id, destination buffer and timeout
 * @when ext_offchain_http_response_read_body_version_1 is invoked on
 * OffchainExtension with request_id, timeout and destination buffer
 * @then On success - returns success and writes response body to dst buffer.
 * Otherwise, returns failue.
 */
TEST_P(HttpResultParametrizedTest, HttpResponseReadBody) {
  WasmI32 request_id = 22;
  Timestamp deadline{300000};
  auto deadline_opt = std::make_optional(deadline);
  Result<uint32_t, HttpError> response(GetParam());
  EXPECT_CALL(*offchain_worker_,
              httpResponseReadBody(request_id, _, deadline_opt))
      .WillOnce(Return(response));

  ASSERT_EQ(
      memory_.decode<decltype(response)>(
          offchain_extension_->ext_offchain_http_response_read_body_version_1(
              request_id, PtrSize{}.combine(), memory_.encode(deadline_opt))),
      response);
}

INSTANTIATE_TEST_SUITE_P(Instance,
                         HttpResultParametrizedTest,
                         ::testing::Values(
                             /// success case
                             200u,
                             /// failure with arbitrary error code
                             HttpError::Timeout)
                         // empty argument for the macro
);

/**
 * @given vector of PeerIds (as buffers)
 * @when ext_offchain_set_authorized_nodes_version_1 is invoked on
 * OffchainExtension with PeerIds
 * @then PeerIds set as authorized
 */
TEST_F(OffchainExtensionTest, SetAuthNodes) {
  std::vector<Buffer> nodes{Buffer("asd"_peerid.toVector())};
  EXPECT_CALL(*offchain_worker_, setAuthorizedNodes(_, true))
      .WillOnce(Return());
  offchain_extension_->ext_offchain_set_authorized_nodes_version_1(
      memory_.encode(nodes), 1);
}

/**
 * @given key, value
 * @when ext_offchain_index_set_version_1 is invoked on
 * OffchainExtension with key and value
 * @then Attempts to write value into offchain storage
 */
TEST_P(OutcomeParameterizedTest, IndexSet) {
  Buffer key(8, 'k');
  Buffer value(8, 'v');
  EXPECT_CALL(*offchain_storage_, set(key.view(), value))
      .WillOnce(Return(GetParam()));
  offchain_extension_->ext_offchain_index_set_version_1(memory_[key],
                                                        memory_[value]);
}

/**
 * @given key
 * @when ext_offchain_index_clear_version_1 is invoked on
 * OffchainExtension with key
 * @then will attempt to remove value from offchain storage
 */
TEST_P(OutcomeParameterizedTest, IndexClear) {
  Buffer key(8, 'k');
  EXPECT_CALL(*offchain_storage_, clear(key.view()))
      .WillOnce(Return(GetParam()));
  offchain_extension_->ext_offchain_index_clear_version_1(memory_[key]);
}

INSTANTIATE_TEST_SUITE_P(Instance,
                         OutcomeParameterizedTest,
                         ::testing::Values<outcome::result<void>>(
                             /// success case
                             outcome::success(),
                             /// failure with arbitrary error code
                             outcome::failure(testutil::DummyError::ERROR))
                         // empty argument for the macro
);
