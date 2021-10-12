/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TEST_MOCK_CORE_STORAGE_TRIE_TRIE_BATCHES_MOCK
#define KAGOME_TEST_MOCK_CORE_STORAGE_TRIE_TRIE_BATCHES_MOCK

#include <gmock/gmock.h>

#include "storage/trie/trie_batches.hpp"

namespace kagome::storage::trie {

  class PersistentTrieBatchMock : public PersistentTrieBatch {
   public:
    MOCK_CONST_METHOD1(get,
                       outcome::result<common::Buffer>(const common::Buffer &));
    MOCK_CONST_METHOD1(tryGet,
                       outcome::result<boost::optional<common::Buffer>>(
                           const common::Buffer &));

    MOCK_METHOD0(trieCursor, std::unique_ptr<PolkadotTrieCursor>());

    MOCK_CONST_METHOD1(contains, bool(const common::Buffer &));

    MOCK_METHOD2(put,
                 outcome::result<void>(const common::Buffer &,
                                       const common::Buffer &));

    outcome::result<void> put(const common::Buffer &k, common::Buffer &&v) {
      return put_rvalueHack(k, std::move(v));
    }
    MOCK_METHOD2(put_rvalueHack,
                 outcome::result<void>(const common::Buffer &, common::Buffer));

    MOCK_METHOD1(remove, outcome::result<void>(const common::Buffer &));

    MOCK_METHOD2(clearPrefix,
                 outcome::result<std::tuple<bool, uint32_t>>(
                     const common::Buffer &buf,
                     boost::optional<uint64_t> limit));

    MOCK_CONST_METHOD0(empty, bool());

    MOCK_METHOD0(commit, outcome::result<storage::trie::RootHash>());

    MOCK_METHOD0(batchOnTop, std::unique_ptr<TopperTrieBatch>());
  };

  class EphemeralTrieBatchMock : public EphemeralTrieBatch {
   public:
    MOCK_CONST_METHOD1(get,
                       outcome::result<common::Buffer>(const common::Buffer &));
    MOCK_CONST_METHOD1(tryGet,
                       outcome::result<boost::optional<common::Buffer>>(
                           const common::Buffer &));

    // issue with gmock when mocks cannot return unique_ptr. Resolved as in
    // https://stackoverflow.com/a/11548191
    MOCK_METHOD0(trieCursorProxy, PolkadotTrieCursor *());
    virtual std::unique_ptr<PolkadotTrieCursor> trieCursor() {
      return std::unique_ptr<PolkadotTrieCursor>(trieCursorProxy());
    }

    MOCK_CONST_METHOD1(contains, bool(const common::Buffer &));

    MOCK_METHOD2(put,
                 outcome::result<void>(const common::Buffer &,
                                       const common::Buffer &));

    outcome::result<void> put(const common::Buffer &k, common::Buffer &&v) {
      return put_rvalueHack(k, std::move(v));
    }
    MOCK_METHOD2(put_rvalueHack,
                 outcome::result<void>(const common::Buffer &, common::Buffer));

    MOCK_METHOD1(remove, outcome::result<void>(const common::Buffer &));

    MOCK_METHOD2(clearPrefix,
                 outcome::result<std::tuple<bool, uint32_t>>(
                     const common::Buffer &buf,
                     boost::optional<uint64_t> limit));

    MOCK_CONST_METHOD0(empty, bool());
  };

  class TopperTrieBatchMock : public TopperTrieBatch {
   public:
    MOCK_METHOD0(writeBack, outcome::result<void>());

    MOCK_CONST_METHOD1(get,
                       outcome::result<common::Buffer>(const common::Buffer &));

    MOCK_CONST_METHOD1(contains, bool(const common::Buffer &));

    MOCK_CONST_METHOD0(empty, bool());

    MOCK_METHOD2(put,
                 outcome::result<void>(const common::Buffer &,
                                       const common::Buffer &));

    outcome::result<void> put(const common::Buffer &k, common::Buffer &&v) {
      return put_rvalueHack(k, std::move(v));
    }
    MOCK_METHOD2(put_rvalueHack,
                 outcome::result<void>(const common::Buffer &, common::Buffer));

    MOCK_METHOD1(remove, outcome::result<void>(const common::Buffer &));

    MOCK_METHOD0(trieCursor, std::unique_ptr<PolkadotTrieCursor>());

    MOCK_METHOD2(clearPrefix,
                 outcome::result<std::tuple<bool, uint32_t>>(
                     const common::Buffer &buf,
                     boost::optional<uint64_t> limit));
  };

}  // namespace kagome::storage::trie

#endif  // KAGOME_TEST_MOCK_CORE_STORAGE_TRIE_TRIE_BATCHES_MOCK
