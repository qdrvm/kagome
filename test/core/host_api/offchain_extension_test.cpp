/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "host_api/impl/offchain_extension.hpp"

#include <gtest/gtest.h>

#include "mock/core/offchain/offchain_persistent_storage_mock.hpp"
#include "mock/core/offchain/offchain_worker_mock.hpp"
#include "mock/core/offchain/offchain_worker_pool_mock.hpp"
#include "mock/core/runtime/memory_mock.hpp"
#include "mock/core/runtime/memory_provider_mock.hpp"
#include "mock/core/runtime/trie_storage_provider_mock.hpp"
#include "mock/core/storage/changes_trie/changes_tracker_mock.hpp"
#include "mock/core/storage/trie/polkadot_trie_cursor_mock.h"
#include "mock/core/storage/trie/trie_batches_mock.hpp"
#include "offchain/types.hpp"
#include "runtime/ptr_size.hpp"
#include "scale/encode_append.hpp"
#include "storage/changes_trie/changes_trie_config.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"
#include "testutil/outcome/dummy_error.hpp"
#include "testutil/prepare_loggers.hpp"

using kagome::common::Buffer;
using kagome::host_api::OffchainExtension;
using kagome::host_api::OffchainExtensionConfig;
using kagome::offchain::Failure;
using kagome::offchain::HttpError;
using kagome::offchain::HttpMethod;
using kagome::offchain::OffchainPersistentStorageMock;
using kagome::offchain::OffchainWorkerMock;
using kagome::offchain::OffchainWorkerPoolMock;
using kagome::offchain::OpaqueNetworkState;
using kagome::offchain::RequestId;
using kagome::offchain::Result;
using kagome::offchain::StorageType;
using kagome::offchain::Success;
using kagome::offchain::Timestamp;
using kagome::primitives::Extrinsic;
using kagome::runtime::Memory;
using kagome::runtime::MemoryMock;
using kagome::runtime::MemoryProviderMock;
using kagome::runtime::PtrSize;
using kagome::runtime::TrieStorageProviderMock;
using kagome::runtime::WasmI32;
using kagome::runtime::WasmI8;
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
    memory_ = std::make_shared<MemoryMock>();
    EXPECT_CALL(*memory_provider_, getCurrentMemory())
        .WillRepeatedly(
            Return(std::optional<std::reference_wrapper<Memory>>(*memory_)));
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
  std::shared_ptr<MemoryMock> memory_;
  std::shared_ptr<MemoryProviderMock> memory_provider_;
  std::shared_ptr<OffchainExtension> offchain_extension_;
  std::shared_ptr<OffchainWorkerMock> offchain_worker_;
  std::shared_ptr<OffchainWorkerPoolMock> offchain_worker_pool_;

  constexpr static uint32_t kU32Max = std::numeric_limits<uint32_t>::max();
};

/// For the tests where it is needed to check a valid behaviour no matter if
/// success or failure was returned by outcome::result
class OutcomeParameterizedTest
    : public OffchainExtensionTest,
      public ::testing::WithParamInterface<outcome::result<void>> {};

class BinaryParameterizedTest : public OffchainExtensionTest,
                                public ::testing::WithParamInterface<WasmI8> {};

class TernaryParametrizedTest : public OffchainExtensionTest,
                                public ::testing::WithParamInterface<WasmI32> {
};

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
  WasmPointer data_pointer = 43;
  WasmSize data_size = 43;
  Extrinsic xt{"data_buffer"_buf};
  WasmSpan data_span = PtrSize(data_pointer, data_size).combine();
  auto result_span = 44;

  EXPECT_CALL(*memory_, loadN(data_pointer, data_size))
      .WillOnce(Return(Buffer{scale::encode(xt).value()}));
  EXPECT_CALL(*offchain_worker_, submitTransaction(_))
      .WillOnce(Return(Success{}));
  EXPECT_CALL(*memory_, storeBuffer(_)).WillOnce(Return(result_span));

  ASSERT_EQ(result_span,
            offchain_extension_->ext_offchain_submit_transaction_version_1(
                data_span));
}

/**
 * @when ext_offchain_network_state_version_1 is invoked on
 * OffchainExtension
 * @then Returns network state
 */
TEST_F(OffchainExtensionTest, NetworkState) {
  auto result_span = 43;
  Result<OpaqueNetworkState, Failure> ons = OpaqueNetworkState();

  EXPECT_CALL(*offchain_worker_, networkState()).WillOnce(Return(ons));
  EXPECT_CALL(*memory_, storeBuffer(_)).WillOnce(Return(result_span));

  ASSERT_EQ(result_span,
            offchain_extension_->ext_offchain_network_state_version_1());
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
  Timestamp result{300000};
  WasmSpan result_span = 42;

  EXPECT_CALL(*offchain_worker_, timestamp()).WillOnce(Return(result));
  {
    auto matcher = [&](const gsl::span<const uint8_t> &data) {
      auto actual_result = scale::decode<Timestamp>(data).value();
      return actual_result == result;
    };
    EXPECT_CALL(*memory_, storeBuffer(Truly(matcher)))
        .WillOnce(Return(result_span));
  }

  ASSERT_EQ(result_span,
            offchain_extension_->ext_offchain_random_seed_version_1());
}

/**
 * @given storage type, key, value
 * @when ext_offchain_local_storage_set_version_1 is invoked on
 * OffchainExtension
 * @then Attempts to write value into local storage
 */
TEST_P(TernaryParametrizedTest, LocalStorageSet) {
  WasmPointer key_pointer = 43;
  WasmSize key_size = 43;
  WasmSpan key_span = PtrSize(key_pointer, key_size).combine();
  Buffer key(8, 'k');
  WasmPointer value_pointer = 44;
  WasmSize value_size = 44;
  WasmSpan value_span = PtrSize(value_pointer, value_size).combine();
  Buffer value(8, 'v');
  EXPECT_CALL(*memory_, loadN(key_pointer, key_size)).WillOnce(Return(key));
  EXPECT_CALL(*memory_, loadN(value_pointer, value_size))
      .WillOnce(Return(value));
  EXPECT_CALL(*offchain_worker_, localStorageSet(_, key, value));
  offchain_extension_->ext_offchain_local_storage_set_version_1(
      GetParam(), key_span, value_span);
}

/**
 * @given storage type, key
 * @when ext_offchain_local_storage_clear_version_1 is invoked on
 * OffchainExtension
 * @then Attempts to remove value from local storage
 */
TEST_P(TernaryParametrizedTest, LocalStorageClear) {
  WasmPointer key_pointer = 43;
  WasmSize key_size = 43;
  WasmSpan key_span = PtrSize(key_pointer, key_size).combine();
  Buffer key(8, 'k');
  EXPECT_CALL(*memory_, loadN(key_pointer, key_size)).WillOnce(Return(key));
  EXPECT_CALL(*offchain_worker_, localStorageClear(_, key));
  offchain_extension_->ext_offchain_local_storage_clear_version_1(GetParam(),
                                                                  key_span);
}

/**
 * @given storage type, key, expected value, value
 * @when ext_offchain_local_storage_compare_and_set_version_1 is invoked on
 * OffchainExtension
 * @then Attempts CAS on local storage
 */
TEST_P(TernaryParametrizedTest, LocalStorageCAS) {
  WasmPointer key_pointer = 43;
  WasmSize key_size = 43;
  WasmSpan key_span = PtrSize(key_pointer, key_size).combine();
  Buffer key(8, 'k');
  WasmPointer value_pointer = 44;
  WasmSize value_size = 44;
  WasmSpan value_span = PtrSize(value_pointer, value_size).combine();
  Buffer value(8, 'v');
  WasmPointer expected_pointer = 45;
  WasmSize expected_size = 45;
  WasmSpan expected_span = PtrSize(expected_pointer, expected_size).combine();
  Buffer expected(8, 'e');
  EXPECT_CALL(*memory_, loadN(key_pointer, key_size)).WillOnce(Return(key));
  EXPECT_CALL(*memory_, loadN(value_pointer, value_size))
      .WillOnce(Return(value));
  EXPECT_CALL(*memory_, loadN(expected_pointer, expected_size))
      .WillOnce(Return(expected));
  EXPECT_CALL(*offchain_worker_, localStorageCompareAndSet(_, key, _, value))
      .WillOnce(Return(true));
  offchain_extension_->ext_offchain_local_storage_compare_and_set_version_1(
      GetParam(), key_span, expected_span, value_span);
}

/**
 * @given storage type, key
 * @when ext_offchain_local_storage_get_version_1 is invoked on
 * OffchainExtension with storage type and key
 * @then Attempts to return value from local storage
 */
TEST_P(TernaryParametrizedTest, LocalStorageGet) {
  WasmPointer key_pointer = 43;
  WasmSize key_size = 43;
  WasmSpan key_span = PtrSize(key_pointer, key_size).combine();
  Buffer key(8, 'k');
  WasmSpan result_span = 44;
  auto result = outcome::success<Buffer>("some_result"_buf);
  auto result_opt = std::make_optional(result.value());

  EXPECT_CALL(*memory_, loadN(key_pointer, key_size)).WillOnce(Return(key));
  EXPECT_CALL(*offchain_worker_, localStorageGet(_, key))
      .WillOnce(Return(result));

  {
    auto matcher = [&](const gsl::span<const uint8_t> &data) {
      auto actual_result = scale::decode<std::optional<Buffer>>(data).value();
      return actual_result == result_opt;
    };

    EXPECT_CALL(*memory_, storeBuffer(Truly(matcher)))
        .WillOnce(Return(result_span));
  }

  ASSERT_EQ(result_span,
            offchain_extension_->ext_offchain_local_storage_get_version_1(
                GetParam(), key_span));
}

INSTANTIATE_TEST_SUITE_P(Instance,
                         TernaryParametrizedTest,
                         testing::Values(0, 1, 2));

// // ext_offchain_http_request_start_version_1
// TEST_P(HttpResultParametrizedTest, HttpRequestStart) {}
//
// // ext_offchain_http_request_add_header_version_1
// TEST_P(HttpResultParametrizedTest, HttpRequestAddHeader) {}
//
// // ext_offchain_http_request_write_body_version_1
// TEST_P(HttpResultParametrizedTest, HttpRequestWriteBody) {}
//
// // ext_offchain_http_response_wait_version_1
// TEST_P(HttpResultParametrizedTest, HttpResponseWait) {}

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
  WasmSpan result_span = 43;
  EXPECT_CALL(*offchain_worker_, httpResponseHeaders(request_id))
      .WillOnce(Return(headers));

  {
    auto matcher = [&](const gsl::span<const uint8_t> &data) {
      auto actual_headers =
          scale::decode<std::vector<std::pair<std::string, std::string>>>(data)
              .value();
      return actual_headers == headers;
    };

    EXPECT_CALL(*memory_, storeBuffer(Truly(matcher)))
        .WillOnce(Return(result_span));
  }
  ASSERT_EQ(result_span,
            offchain_extension_->ext_offchain_http_response_headers_version_1(
                request_id));
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
  WasmPointer dst_pointer = 43;
  WasmSize dst_size = 43;
  Buffer dst(43, 0);
  WasmPointer deadline_pointer = 43;
  WasmSize deadline_size = 43;
  Timestamp deadline{300000};
  auto deadline_opt = std::make_optional(deadline);
  Result<uint32_t, HttpError> response(GetParam());
  WasmSpan result = 44;
  gsl::span<const unsigned char> dst_buf = gsl::make_span(dst);
  EXPECT_CALL(*memory_, loadN(deadline_pointer, deadline_size))
      .WillOnce(
          Return(Buffer{gsl::make_span(scale::encode(deadline_opt).value())}));
  EXPECT_CALL(*offchain_worker_,
              httpResponseReadBody(request_id, dst, deadline_opt))
      .WillOnce(Return(response));
  if (response.isSuccess()) {
    EXPECT_CALL(*memory_, storeBuffer(dst_pointer, dst_buf)).WillOnce(Return());
  }

  {
    auto matcher = [&](const gsl::span<const uint8_t> &data) {
      auto actual_response =
          scale::decode<Result<uint32_t, HttpError>>(data).value();
      return actual_response.isSuccess() == response.isSuccess();
    };

    EXPECT_CALL(*memory_, storeBuffer(Truly(matcher))).WillOnce(Return(result));
  }

  ASSERT_EQ(result,
            offchain_extension_->ext_offchain_http_response_read_body_version_1(
                request_id,
                PtrSize{dst_pointer, dst_size}.combine(),
                PtrSize{deadline_pointer, deadline_size}.combine()));
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
  WasmPointer nodes_pos_pointer = 43;
  WasmSize nodes_pos_size = 43;
  std::vector<Buffer> nodes{Buffer("asd"_peerid.toVector())};
  EXPECT_CALL(*memory_, loadN(nodes_pos_pointer, nodes_pos_size))
      .WillOnce(Return(Buffer{gsl::make_span(scale::encode(nodes).value())}));
  EXPECT_CALL(*offchain_worker_, setAuthorizedNodes(_, true))
      .WillOnce(Return());
  offchain_extension_->ext_offchain_set_authorized_nodes_version_1(
      PtrSize{nodes_pos_pointer, nodes_pos_size}.combine(), 1);
}

/**
 * @given key, value
 * @when ext_offchain_index_set_version_1 is invoked on
 * OffchainExtension with key and value
 * @then Attempts to write value into offchain storage
 */
TEST_P(OutcomeParameterizedTest, IndexSet) {
  WasmPointer key_pointer = 43;
  WasmSize key_size = 43;
  Buffer key(8, 'k');
  WasmPointer value_pointer = 44;
  WasmSize value_size = 44;
  Buffer value(8, 'v');
  EXPECT_CALL(*memory_, loadN(key_pointer, key_size)).WillOnce(Return(key));
  EXPECT_CALL(*memory_, loadN(value_pointer, value_size))
      .WillOnce(Return(value));
  EXPECT_CALL(*offchain_storage_, set(key, value)).WillOnce(Return(GetParam()));
  offchain_extension_->ext_offchain_index_set_version_1(
      PtrSize{key_pointer, key_size}.combine(),
      PtrSize{value_pointer, value_size}.combine());
}

/**
 * @given key
 * @when ext_offchain_index_clear_version_1 is invoked on
 * OffchainExtension with key
 * @then will attempt to remove value from offchain storage
 */
TEST_P(OutcomeParameterizedTest, IndexClear) {
  WasmPointer key_pointer = 43;
  WasmSize key_size = 43;
  Buffer key(8, 'k');
  EXPECT_CALL(*memory_, loadN(key_pointer, key_size)).WillOnce(Return(key));
  EXPECT_CALL(*offchain_storage_, clear(key)).WillOnce(Return(GetParam()));
  offchain_extension_->ext_offchain_index_clear_version_1(
      PtrSize{key_pointer, key_size}.combine());
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
