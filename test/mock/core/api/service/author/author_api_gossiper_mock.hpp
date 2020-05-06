/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TEST_CORE_API_SERVICE_AUTHOR_API_GOSSIPER_MOCK_HPP
#define KAGOME_TEST_CORE_API_SERVICE_AUTHOR_API_GOSSIPER_MOCK_HPP

#include <gmock/gmock.h>

#include "api/service/author/author_api_gossiper.hpp"

namespace kagome::api {

  class AuthorApiGossiperMock : public AuthorApiGossiper {
   public:
    ~AuthorApiGossiperMock() override = default;

    MOCK_METHOD1(transactionAnnounce,
                 void(const network::TransactionAnnounce &));
  };

}  // namespace kagome::api

#endif  // KAGOME_TEST_CORE_API_SERVICE_AUTHOR_API_GOSSIPER_MOCK_HPP
