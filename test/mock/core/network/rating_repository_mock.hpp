/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_RATING_REPOSITORY_MOCK_HPP
#define KAGOME_RATING_REPOSITORY_MOCK_HPP

#include <gmock/gmock.h>

#include "network/rating_repository.hpp"

namespace kagome::network {

  class PeerRatingRepositoryMock : public PeerRatingRepository {
   public:
    MOCK_METHOD(PeerScore, rating, (const PeerId &), (const override));

    MOCK_METHOD(PeerScore, upvote, (const PeerId &), (override));

    MOCK_METHOD(PeerScore, downvote, (const PeerId &), (override));

    MOCK_METHOD(PeerScore, update, (const PeerId &, PeerScore), (override));

    MOCK_METHOD(PeerScore,
                upvoteForATime,
                (const PeerId &, std::chrono::seconds),
                (override));

    MOCK_METHOD(PeerScore,
                downvoteForATime,
                (const PeerId &, std::chrono::seconds),
                (override));

    MOCK_METHOD(PeerScore,
                updateForATime,
                (const PeerId &, PeerScore, std::chrono::seconds),
                (override));
  };

}  // namespace kagome::network

#endif  // KAGOME_RATING_REPOSITORY_MOCK_HPP
